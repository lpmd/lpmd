#!/usr/bin/env python

from povscene import *
from parselpmd2 import LPMD2
from sys import stdin
import sys
from os import system
from math import *

################
#DEFINITIONS####
################
def norma(x): return sqrt(sum(z**2 for z in x))
def vecno(v): return [v[0]/norma(v), v[1]/norma(v), v[2]/norma(v)]
def dot(x,y): return sum(a*b for a,b in zip(x,y))
def distance(x,y): return sqrt(sum((a-b)**2 for a,b in zip(x,y)))
def vdist(x,y): return(y[0]-x[0],y[1]-x[1],y[2]-x[2])
#Cross Defined with left-hand (povray style)
def cross(x,y): 
 a = x[1]*y[2]-x[2]*y[1]
 b = x[0]*y[2]-x[2]*y[0]
 c = x[0]*y[1]-x[1]*y[0]
 return (-a,-b,-c)
#Global variables
lp = LPMD2()
c = 1
option = {'null': 1}

def Movie(formatinput="png"):
    fcode = "" 
    if option['movie']['format']=="avi": fcode = " -ovc lavc -lavcopts vcodec=mpeg4:vqscale=2:vhq:trell:autoaspect "
    if option['movie']['format']=="mpg": fcode = " -ovc lavc -lavcopts vcodec=wmv2 "
    command = "mencoder mf://*.%s -mf fps=%d -o %s %s" % (formatinput, float(option['movie']['value']), option['movie']['file'], fcode)
    os.system(command)

def render(lp, c):
    background = option['background']['color']
    tmp = option['size']['size'].split('x')
    sizes = (int(tmp[0]), int(tmp[1]))
    LX = norma(lp.cell[0]) 
    LY = norma(lp.cell[1])
    LZ = norma(lp.cell[2])

    vec1 = option['cameraLocation']['position']
    vec2 = option['cameraLookat']['position']
    lvec = distance(vec1,vec2)
    mvec = ( (float(vec2[0])-float(vec1[0]))*0.1 , (float(vec2[1])-float(vec1[1]))*0.1 , (float(vec2[2])-float(vec1[2]))*0.8 )
    lightpos = (mvec[0]+2.0*lvec,mvec[1]+2.0*lvec,mvec[2]+lvec*1.5)
    cameraLocation = option['cameraLocation']['position'] 
    cameraUp = option['cameraUp']['position']
    cameraLookat = option['cameraLookat']['position']
    lightSource1= lightpos
    lightSource2= option['cameraLocation']['position']

    #aspect negative by left-hand rule
    aspect = -float(float(sizes[0])/float(sizes[1]))
    scene = Scene()
    camarg = {'camera':str(option['camera']['value'])}
    if("cameraAngle" in option): camarg['angle']=float(option['cameraAngle']['value'])
    if(camarg['camera']=="orthographic"):
     up = (cameraUp[0]*sizes[1]*0.5,cameraUp[1]*sizes[1]*0.5,cameraUp[2]*sizes[1]*0.5)
     if (cameraLocation[2]<=0):
      tmpr1 = cross(up,vdist(vec2,vec1))
     else:
      tmpr1 = cross(up,vdist(vec1,vec2))
     tmpr  = vecno(tmpr1)
     right = (tmpr[0]*sizes[0]*0.5,tmpr[1]*sizes[0]*0.5,tmpr[2]*sizes[0]*0.5)
     camarg['up']=str(up)
     camarg['aspect']=str(right)
    if(camarg['camera']=="perspective"):
     camarg['aspect']=aspect
    camarg['sky']=cameraUp
    scene.Add(Camera(location=cameraLocation, direction=cameraLookat, **camarg))

    scene.SetBackgroundColor("<%f, %f, %f>" % background)
    scene.AddLight(LightSource(lightSource1, shadowless=False, spot=False))
    scene.AddLight(LightSource(lightSource2, shadowless=False, spot=False))
    scene.SetAmbientLight(True)

    print "Number of atoms in scene ", c, " is ", len(lp)
    for atom in range(len(lp)):
        (x, y, z) = lp.PackTags(('X','Y','Z'), atom)
        x = x*LX
        y = y*LY
        z = z*LZ
        (r, g, b) = lp.Tag('rgb', atom)
        radius = float(option['radius']['value'])
        sphere = Sphere((x, y, z), radius)
        sphere.SetColor("<%f, %f, %f>" % (r, g, b))
        sphere.SetSpecular(0.1)
        sphere.SetPhong(0.3)
        scene.Add(sphere)
    if (option['box']['value']==True):
     borders = Cube(lp.cell[0],lp.cell[1],lp.cell[2],r=float(option['box']['radius']))
     borders.SetColor("<%f, %f, %f>" % option['box']['color'])
     borders.SetSpecular(0.1)
     borders.SetPhong(0.3)
     scene.Add(borders)
    print "Rendering movie%.4d..." % (c)
    if ("povfiles" in option):
     if(option['povfiles']['value']=="keep"): scene.ExportPOV("movie%.4d.pov" % c)
    scene.Render("movie%.4d.png" % c, format="png", size=sizes, antialias=True, op=option)


def newrender(lp):
 global c
 render(lp,c)
 c += 1
def RunRender(o):
 global option
 option=o
 lp.ReadInPlace(option['input']['file'],newrender)
 if ("movie" in option):
  Movie(formatinput="png")

