#!/usr/bin/env python
# coding: utf-8

# In[1]:


# Modules
import numpy as np
import json


# In[2]:


def getBits(value, nBits, endianType="little"): 
  return np.array([((value & (1 << i)) >> i) for i in range(nBits)])[::(int(endianType=="little")*2-1)]

# Get ID based on bits and endianness
def getID(bits, endianType="little"): 
  return np.sum(np.power(2, np.nonzero(np.squeeze(np.array(bits))[::(int(endianType=="little")*2-1)])))

def normalizeID(ID, nBits):
  # Normalize between -1 and 1
  return ID / pow(2,nBits) * 2.0 - 1.0

# series of bits with a stop bit at the beginning
def getBaseSequence(ID, nBits, endianType="little", bIncludeStop=True):
  out = getBits(ID, nBits, endianType)
  if bIncludeStop:
    out = np.insert(out, 0, -1)
  return out

def getPulseLengthEncoding(sequence, pulseStartBit = 0, offLength = 1, onLength = 2, stopLength = 5):
  a = pulseStartBit
  b = 1 - a
  return np.hstack(np.array([np.array([a]+[b]*((i==-1)*stopLength+(i==0)*offLength+(i==1)*onLength)) for i in sequence]))

def getSwitchEncoding(sequence, startBit = 1, stopLength = 3):
  a = startBit
  b = 1 - a
  return np.hstack(np.array([np.array([a]+[(a*(i==1)+b*(i<=0))]*((i!=-1)*1+(i==-1)*stopLength)) for i in sequence]))

def getNonEncodedSequence(sequence):
  return sequence

def isMaxEncodingPossible(sequence, nMaxZeros=2):
    seq = np.delete(sequence, np.where(sequence == -1))
    maxZeroSeq = ''.join('0' for x in range(nMaxZeros+1))
    bitString =''.join(str(abs(x)) for x in seq)
    return maxZeroSeq not in bitString

def getMaxEncodedSequence(sequence, nMaxZeros=2, stopLength=3):
    if not isMaxEncodingPossible(sequence, nMaxZeros): return None
#     return np.hstack(np.array([[i*(i>=0)+0*(i<0)]*(1*(i>=0)+stopLength*(i<0)) for i in sequence]))
    stop = [1]+[0]*stopLength+[1]
    out = []
    for i in sequence:
        if i >= 0: out.append(i)
        else: out.append(stop)
    return np.hstack(np.array(out))

def getRandomMaxEncoding(nBits, nMaxZeros=2, stopLength=3):
    out = []
    for i in range(nBits):
        if i == 0 or out[-1] == 1:
            out.append(np.random.randint(2))
        elif np.sum(out[max(len(out)-nMaxZeros,0):len(out)])==0:
            out.append(1)
        else:
            out.append(np.random.randint(2))
    out.append([1]+[0]*stopLength+[1])
    return np.hstack(np.array(out))
    
def getRandomMaxEncodingID(nBits, nMaxZeros=2, stopLength=3):
    return getID(getRandomMaxEncoding(nBits, nMaxZeros, stopLength))

def getMaxNumRolledZeros(sequence):
    nMaxZeros = 0
    for i in range(len(sequence)):
        seq = np.roll(sequence, i)
        seqSum = int(np.sum(seq))
        bitString =''.join(str(abs(x)) for x in seq)
        nZerosTest = len(sequence)
        minZeros = 0
        while nZerosTest >= minZeros:
            zeroSeq = ''.join('0' for x in range(nZerosTest))
            if zeroSeq in bitString:
                nMaxZeros = nZerosTest
                break
            nZerosTest -= 1
    return nMaxZeros 

def hasLTOETRolledZeros(sequence, nMaxZeros): # Less than or equal to
    bitString =''.join(str(abs(x)) for x in sequence)
    zeroSeq = ''.join('0' for x in range(nMaxZeros+1))
    if zeroSeq in bitString:
        return False
    nZeros = getMaxNumRolledZeros(sequence)
    return nZeros <= nMaxZeros

# Does a sequence have a subsequence in any of its rolls?
def hasRolledSubsequence(sequence, subseq, nBits):
    for i in range(nBits):
        seq = np.roll(sequence, i)
        bitString =''.join(str(abs(x)) for x in seq)
        if subseq in bitString:
            return True
    return False

def getEncodedSequence(sequence, mode='pulse-length'):
  if mode == 'pulse-length':
    return getPulseLengthEncoding(sequence)
  elif mode == 'switch':
    return getSwitchEncoding(sequence)
  elif mode == 'none':
    return getNonEncodedSequence(sequence)
  elif mode == 'max':
    return getMaxEncodedSequence(sequence)
  return getNonEncodedSequence(sequence)


# In[4]:


# This function calculates all possible obsserved IDs from Motive, given a lampID
def getObsIDs(lampID, nBitsLampID=12, startSeq=np.array([1,0,0,1])):
    lampSeq = np.append(getBits(lampID, nBitsLampID),startSeq)
    lampSeqInvRev = np.flip(1-lampSeq)
    obsIDs = []
    nBitsLampSeq = nBitsLampID+len(startSeq)
    for i in range(nBitsLampSeq):
        seq = np.roll(lampSeqInvRev, i)
        obsIDs.append(getID(seq))
    obsIDs.sort()
    return obsIDs


# In[6]:


# Find all possible lampIDs

# User params:
nBits = 16

# Do not change:
nMaxSequentialZeros = 2
nMinTotalZeros = 1
subSeq = '00'

nTotalIDs = pow(2, nBits)
ref = np.zeros(nTotalIDs).astype(int)
ref.fill(-1) # -2 = invalid, -1 = undetermined, >= 0 = pointer to that ID
uniqueIDs = [] # Full sequence ID
lampIDs = [] # Lamp ID

for i in range(nTotalIDs):
    if ref[i] == -2:
        # invalid
        continue
    if ref[i] >= 0:
        # already marked with a pointer
        continue
    
    # Get this sequence
    seq = getBits(i, nBits)
    
    # Check if this sequence is valid or invalid
    if nBits-np.sum(seq) < nMinTotalZeros or not hasLTOETRolledZeros(seq, nMaxSequentialZeros) or not hasRolledSubsequence(seq, subSeq, nBits):
        # This ID is invalid
        # Mark all rolls of this sequence as invalid
        for r in range(nBits):
            ref[getID(np.roll(seq, r))] = -2
    else:
        # This ID is valid
        # Find the lowest possible ID
        lowestID = nTotalIDs+1
        for r in range(nBits):
            ID = getID(np.roll(seq, r))
            if ID < lowestID: lowestID = ID
        # The lowest ID is valid and all others will point to this one
        for r in range(nBits):
            ref[getID(np.roll(seq, r))] = lowestID
        # Save this lowest ID
        uniqueIDs.append(lowestID)
        lampIDs.append(getID(np.roll(getBits(i, nBits),-1)[:-4]))

print(len(uniqueIDs), " possible IDs")

# print("[sequence]","fullID","lampID")
# for i, ID in enumerate(uniqueIDs):
#     print(getBits(ID, nBits), ID, lampIDs[i], hex(lampIDs[i]))
    
# uniqueLampIDs contains all 940 possible lampIDs


# In[7]:


# Create a dictionary that maps all possible obsIDs to lampIDs. 
# If a obsID is not possible, then the lampID will be -1.
obs2lampID = np.zeros(pow(2,16)).astype(int)
obs2lampID.fill(-1)
for lampID in lampIDs:
    obsIDs = getObsIDs(lampID)
    for obsID in obsIDs:
        obs2lampID[obsID] = lampID


# In[33]:


# Save the dictionary to json file
out = {}
out["nBits"] = nBits
out["dict"] = [int(i) for i in obs2lampID]

with open('id-dictionary.json', 'w') as outfile:
    json.dump(out, outfile)

