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
def sumlist(x,y): return(y[0]+x[0],y[1]+x[1],y[2]+x[2])
def factorl(x,y): return(x[0]*y,x[1]*y,x[2]*y)
def inside(p,a,b,c):
 wp = (a,b,c)
 for i in range(0,3):
  if (p[i] < 0.0 or p[i]>norma(wp[i])): return 1 
 return 0
#Cross Defined with left-hand (povray style)
def cross(x,y): 
 a = x[1]*y[2]-x[2]*y[1]
 b = x[0]*y[2]-x[2]*y[0]
 c = x[0]*y[1]-x[1]*y[0]
 return (a,-b,c)

def Movie(formatinput="png"):
    fcode = "" 
    if option['movie']['format']!="gif":
     if option['movie']['format']=="avi": fcode = " -ovc lavc -lavcopts vcodec=mpeg4:vqscale=2:vhq:trell:autoaspect "
     if option['movie']['format']=="mpg": fcode = " -ovc lavc -lavcopts vcodec=wmv2 "
     command = "mencoder mf://*.%s -mf fps=%d -o %s %s" % (formatinput, option['movie']['value'], option['movie']['file'], fcode)
    else:
     command = "convert -loop 0 -delay %s *.%s %s" % (option['movie']['value'], formatinput, option['movie']['file'])
    print "Generating Movie .... please wait."
    os.system(command)

def render(lp, c):
    background = option['background']['color']
    tmp = option['size']['size'].split('x')
    sizes = (int(tmp[0]), int(tmp[1]))
    LX = norma(lp.cell[0]) 
    LY = norma(lp.cell[1])
    LZ = norma(lp.cell[2])
    scene = Scene()

    #Start to rotate
    if ("startrotate" in option):
     sr = int(option['startrotate']['value'])
    else :
     sr = 1
    #Endrotate
    if ("endrotate" in option):
     er = int(option['endrotate']['value'])
    else :
     er = -1
    #cp=cameraLocation;cu=cameraUp;cl=cameraLookat;lc=LargeBetweenCLandCP
    cp = option['cameraLocation']['position'] 
    cu = option['cameraUp']['position']
    cl = option['cameraLookat']['position']
    lc = distance(cp,cl)

    #CAMERA SECTION.
    #aspect negative by left-hand rule
    aspect = -float(float(sizes[0])/float(sizes[1]))
    camepo = [cp[0], cp[1], cp[2]]
    camelo = [cl[0], cl[1], cl[2]]
    fcl = (camelo[0], camelo[1], camelo[2]) 
    fcp = (camepo[0], camepo[1], camepo[2])
    
    ligarg = {'color':'White'}
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
    if(("cameraRotate" in option) and c>=sr):
     val = float(option["cameraRotate"]["value"])*float(c-sr)
     if(c>=er and er!=-1):
      val=float(option["cameraRotate"]["value"])*float(er-sr)
     camarg['rotatepos'] = option["cameraRotate"]["poslook"]
     camarg['rotatevec'] = option["cameraRotate"]["rotvect"]
     camarg['rotatefac'] = str(val)
     ligarg['rotatepos'] = camarg['rotatepos']
     ligarg['rotatevec'] = camarg['rotatevec']
     ligarg['rotatefac'] = camarg['rotatefac']
     print "Rotating camera by " + str(val) + " ... " 
    scene.Add(Camera(location=fcp, direction=fcl, **camarg))

    #LIGHTS SECTION.
    P = vdist(cp,cl)
    q = vecno(cu)
    p = vecno(P)
    r = cross(p,q)
    rp = cross(q,p)
    qp = cross(p,r)
    pp = cross(r,q)
    v1 = sumlist(cl, factorl(P,0.3333))
    v2 = sumlist(cl, factorl(P,-0.3333))
    fs = max((LX,LY,LZ))*1.50
    if("cameraLight" in option):
     fs = float(option['cameraLight']['value'])
    lightSource0 = option['cameraLocation']['position']
    lightSource1 = sumlist(v1,factorl(q,fs))
    lightSource2 = sumlist(v1,factorl(qp,fs))
    lightSource3 = sumlist(v1,factorl(p,fs))
    lightSource4 = sumlist(v1,factorl(pp,fs))
    if(("warning" in option)==False or option['warning']['value']=="on"):
     for i in range(1,5):
      light="lightSource"+str(i)
      if(inside(vars()[light],lp.cell[0],lp.cell[1],lp.cell[2])==0):
       print " WARNING : Light " + str (i) + " = " + str(vars()[light])
       print "           is inside the cell."

    scene.AddLight(LightSource(lightSource1, shadowless=False, spot=False, **ligarg))
    scene.AddLight(LightSource(lightSource2, shadowless=False, spot=False, **ligarg))
    scene.AddLight(LightSource(lightSource3, shadowless=False, spot=False, **ligarg))
    scene.AddLight(LightSource(lightSource4, shadowless=False, spot=False, **ligarg))
    scene.AddLight(LightSource(lightSource0, shadowless=False, spot=False, **ligarg))
    scene.SetAmbientLight(True)

    if("extraLight" in option):
     if(option['extraLight']['value']=="back"):
      lightSource5 = sumlist(v2,factorl(q,fs))
      lightSource6 = sumlist(v2,factorl(qp,fs))
      lightSource7 = sumlist(v2,factorl(p,fs))
      lightSource8 = sumlist(v2,factorl(pp,fs))
      lightSource9 = factorl(cl,-1.0)
      scene.AddLight(LightSource(lightSource5, shadowless=False, spot=False, **ligarg))
      scene.AddLight(LightSource(lightSource6, shadowless=False, spot=False, **ligarg))
      scene.AddLight(LightSource(lightSource7, shadowless=False, spot=False, **ligarg))
      scene.AddLight(LightSource(lightSource8, shadowless=False, spot=False, **ligarg))
      scene.AddLight(LightSource(lightSource9, shadowless=False, spot=False, **ligarg))

    scene.SetBackgroundColor("<%f, %f, %f>" % background)

    #PLANES SECTION.
    ab = sumlist(lp.cell[0],lp.cell[1])
    bc = sumlist(lp.cell[1],lp.cell[2])
    ac = sumlist(lp.cell[0],lp.cell[2])
    abc = sumlist(ab,lp.cell[2])
    allplanes = ( ((0,0,0),lp.cell[0],ab,lp.cell[1]),
                  ((0,0,0),lp.cell[0],ac,lp.cell[2]),
                  ((0,0,0),lp.cell[2],bc,lp.cell[1]),
                  (sumlist((0,0,0),lp.cell[2]),sumlist(lp.cell[0],lp.cell[2]),sumlist(ab,lp.cell[2]),sumlist(lp.cell[1],lp.cell[2])),
                  (sumlist((0,0,0),lp.cell[1]),sumlist(lp.cell[0],lp.cell[1]),sumlist(ac,lp.cell[1]),sumlist(lp.cell[2],lp.cell[1])),
                  (sumlist((0,0,0),lp.cell[0]),sumlist(lp.cell[2],lp.cell[0]),sumlist(bc,lp.cell[0]),sumlist(lp.cell[1],lp.cell[0])),
                )
 
    for i in range(1,7):
     plane = "plane" + str(i)
     if(plane in option):
      bplane = BoxPlane(allplanes[i-1])
      bplane.SetColor("<%f, %f, %f>" % option[plane]['color'])
      bplane.SetSpecular(0.01)
      bplane.SetPhong(0.03)
      bplane.SetTransmit(float(float(option[plane]['transparency'])/100.0))
      scene.Add(bplane)

    #ATOMS SECTION.
    print "Number of atoms in scene ", c, " is ", len(lp)
    for atom in range(len(lp)):
        (x, y, z) = lp.PackTags(('X','Y','Z'), atom)
        x = x*LX
        y = y*LY
        z = z*LZ
        radius = float(option['radius']['value'])
        sphere = Sphere((x, y, z), radius)
        if ('rgb' in lp.tags):
         sphere.SetColor("<%f, %f, %f>" % lp.Tag('rgb', atom))
        else:
         if ("atomcolor" in option): 
          sphere.SetColor("<%f, %f, %f>" % option['atomcolor']['color'])
         else:
          sphere.SetColor("<0.0,0.0,1.0>")
        sphere.SetSpecular(0.1)
        sphere.SetPhong(0.3)
        scene.Add(sphere)
    if ("box" in option):
     borders = Cube(lp.cell[0],lp.cell[1],lp.cell[2],r=float(option['box']['radius']))
     borders.SetColor("<%f, %f, %f>" % option['box']['color'])
     borders.SetSpecular(0.1)
     borders.SetPhong(0.3)
     scene.Add(borders)
    fmt =''
    if("format" in option):
     fmt = option['format']['value']
    else:
     fmt = "movie%.4d"
    ffmt = fmt % (c) + ".png"
    print "Rendering %s..." % (ffmt)
    if ("povfiles" in option):
     if(option['povfiles']['value']=="keep"): scene.ExportPOV("movie%.4d.pov" % c)
    scene.Render(ffmt, format="png", size=sizes, antialias=True, op=option)


def newrender(lp):
 global c
 if (("startframes" in option) and c==1):
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

