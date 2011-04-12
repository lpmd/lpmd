#!/usr/bin/env python

from povscene import *
from parselpmd2 import LPMD2
from sys import stdin
import sys
from os import system
from math import *

####################
#Global variables###
####################
lp = LPMD2()
c = 1
option = {'null': 1}

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

    #cp=cameraLocation;cu=cameraUp;cl=cameraLookat;lc=LargeBetweenCLandCP
    cp = option['cameraLocation']['position'] 
    cu = option['cameraUp']['position']
    cl = option['cameraLookat']['position']
    lc = distance(cp,cl)
    mvec = ((float(cl[0])-float(cp[0]))*0.5, (float(cl[1])-float(cp[1]))*0.5, (float(cl[2])-float(cp[2]))*0.5)
    lightpos0 = (cl[0]+cu[0]*lc*0.5,cl[1]+cu[1]*lc*0.5,cl[2]+cu[2]*lc*0.5)
    lightpos1 = (cl[0]+cu[0]*lc*0.5,cl[1]+cu[1]*lc*0.5,cl[2]-cu[2]*lc*0.5)
    lightpos2 = (cl[0]+cu[0]*lc*0.5,cl[1]-cu[1]*lc*0.5,cl[2]+cu[2]*lc*0.5)
    lightpos3 = (cl[0]-cu[0]*lc*0.5,cl[1]+cu[1]*lc*0.5,cl[2]+cu[2]*lc*0.5)
    lightSource0= lightpos0
    lightSource1= lightpos1
    lightSource2= lightpos2
    lightSource3= lightpos3
    lightSource4= option['cameraLocation']['position']

    #aspect negative by left-hand rule
    aspect = -float(float(sizes[0])/float(sizes[1]))
    camepo = [cp[0], cp[1], cp[2]]
    camelo = [cl[0], cl[1], cl[2]]
    fcl = (camelo[0], camelo[1], camelo[2]) 
    fcp = (camepo[0], camepo[1], camepo[2])
    
    scene = Scene()
    camarg = {'camera':str(option['camera']['value'])}
    if("cameraAngle" in option): camarg['angle']=float(option['cameraAngle']['value'])
    if(camarg['camera']=="orthographic"):
     angle = atan(float(sizes[0])/float(lc))*180.0/pi
     camarg['angle']=angle 
     camarg['aspect']=aspect
     camarg['sky']=cu
    if(camarg['camera']=="perspective"):
     camarg['aspect']=aspect
     camarg['sky']=cu
    if("cameraRotate" in option):
     val = float(option["cameraRotate"]["value"])*float(c)
     camarg['rotatepos'] = option["cameraRotate"]["poslook"]
     camarg['rotatevec'] = option["cameraRotate"]["rotvect"]
     camarg['rotatefac'] = str(val)
    scene.Add(Camera(location=fcp, direction=fcl, **camarg))

    scene.SetBackgroundColor("<%f, %f, %f>" % background)
    scene.AddLight(LightSource(lightSource0, shadowless=False, spot=False))
    scene.AddLight(LightSource(lightSource1, shadowless=False, spot=False))
    scene.AddLight(LightSource(lightSource2, shadowless=False, spot=False))
    scene.AddLight(LightSource(lightSource3, shadowless=False, spot=False))
    scene.AddLight(LightSource(lightSource4, shadowless=False, spot=False))
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
 if ("startframes" in option and c==1):
  for i in range (1,int(option['startframes']['value'])):
   render (lp,c)
   c+=1
 render(lp,c)
 c += 1
def RunRender(o):
 global option
 global c
 option=o
 tmplp = [None]
 lp.ReadInPlace(option['input']['file'],tmplp,newrender)
 if ("finalframes" in option):
  for i in range(1,int(option['finalframes']['value'])):
   render (tmplp[0],c)
   c+=1
 if ("movie" in option):
  Movie(formatinput="png")

