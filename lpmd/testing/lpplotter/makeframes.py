#!/usr/bin/env python

from povscene import *
from parselpmd2 import LPMD2
from sys import stdin
import sys
from os import system
from math import *

#
#
#
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
    LX = sqrt(lp.cell[0][0]**2 + lp.cell[0][1]**2 + lp.cell[0][2]**2)
    LY = sqrt(lp.cell[1][0]**2 + lp.cell[1][1]**2 + lp.cell[1][2]**2)
    LZ = sqrt(lp.cell[2][0]**2 + lp.cell[2][1]**2 + lp.cell[2][2]**2)

    vec1 = option['cameraLocation']['position']
    vec2 = option['cameraLookat']['position']
    lvec = sqrt((float(vec2[0])-float(vec1[0]))**2 + (float(vec2[1])-float(vec1[1]))**2 + (float(vec2[2])-float(vec1[2]))**2 )
    mvec = ( (float(vec2[0])-float(vec1[0]))*0.1 , (float(vec2[1])-float(vec1[1]))*0.1 , (float(vec2[2])-float(vec1[2]))*0.8 )
    lightpos = (mvec[0]+2.0*lvec,mvec[1]+2.0*lvec,mvec[2]+lvec*1.5)
    cameraLocation = option['cameraLocation']['position'] 
    cameraLookat = option['cameraLookat']['position']
    lightSource1= lightpos
    lightSource2= option['cameraLocation']['position']

    aspect = float(float(sizes[0])/float(sizes[1]))
    scene = Scene()
    if("cameraAngle" in option):
     scene.Add(Camera(location=cameraLocation, direction=cameraLookat, camera=str(option['camera']['value']),angle=float(option['cameraAngle']['value']),aspect=aspect,))
    else:
     scene.Add(Camera(location=cameraLocation, direction=cameraLookat, camera=str(option['camera']['value']),aspect=aspect,))

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

