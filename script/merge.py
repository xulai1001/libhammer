#!/usr/bin/python
#-*- coding: UTF-8 -*-

import re, os, sys

#open file
#f_one=open("test1.txt",'r')
#f_two=open("test2.txt",'r')
f_new=open("test3.txt",'w')
#f_one=open("file_1",'r')
#f_two=open("file_2",'r')
#f_new=open("file_n",'w')


#path='/home/less/tmp/'
#flist=os.listdir(path)
#f_two=open('/home/less/tmp/'+flist[0], 'r')
#f_one=open('/home/less/tmp/'+flist[1], 'r')


f_two=open(sys.argv[1], 'r')
f_one=open(sys.argv[2], 'r')

class HammerResult:
    def set(self, base, offset, p, q, value, flip_to):
        self.base=base
        self.offset=offset
        self.p=p
        self.q=q
        self.value=value
        self.flip_to=flip_to

    def from_str(self, s):
        l=s.split(',')
        self.set(int(l[0],16), int(l[1]), int(l[2],16), int(l[3],16), int(l[4],16), int(l[5]))

    def to_str(self):
        return "0x%x, %d, 0x%x, 0x%x, 0x%x, %d" % (self.base, self.offset, self.p, self.q, self.value, self.flip_to)

    def order(self, li):
        i=0
        while i<len(li):
            if li[i].base>li[i+1].base:
                print "base is False"
                break
            i+=1
        if i==len(li):
            print "base is True"

        i=0
        while i<len(li)-1:
            if li[i].p>li[i+1].p:
                print "p is False"
                break
            i+=1
        if i==len(li)-1:
            print "p is True"

        i=0
        while i<len(li)-1:
            if li[i].q>li[i+1].p:
                print "q is False"
                break
            i+=1
        if i==len(li)-1:
            print "q is True"

    #def __eq__(self, other):
        #return self.p==other.p and self.base==other.base and self.q==other.q and self.offset==other.offset and self.flip_to==other.flip_to

    #def __hash__(self):
        #return self.to_str().__hash__()

def mergelist(l1,l2):
    tmp=[]
    while len(l1)>0 and len(l2)>0:
        if l1[0].p<l2[0].p:
            tmp.append(l1[0])
            del l1[0]
        elif l1[0].p==l2[0].p:
            tmp.append(l1[0])
            del l1[0]
            del l2[0]
        else:
            tmp.append(l2[0])
            del l2[0]
    tmp.extend(l1)
    tmp.extend(l2)
    return tmp




#read line
f_one_lines=f_one.readlines()
f_two_lines=f_two.readlines()

li1=[]
li2=[]
li3=[]
for line1 in f_one_lines:
    piece=HammerResult()
    piece.from_str(line1)
    li1.append(piece)
for line2 in f_two_lines:
    piece=HammerResult()
    piece.from_str(line2)
    li2.append(piece)

#set1=set(li1)
#set2=set(li2)
#set3=set1|set2


li3=mergelist(li1, li2)

count=0
for elem in li3:
    f_new.write(elem.to_str()+'\n')
    count+=1
print count

#close file
f_one.close()
f_two.close()
f_new.close()

