
#
#
#

import os 
from math import *

class Object:
  
  def __init__(self):
      self.phong = 0
      self.specular = 0
      self.transmit = 1
      self.color = "Green"

  def SetColor(self, color):
      self.color = color

  def SetPhong(self, phong):
      self.phong = phong 

  def SetSpecular(self, specular):
      self.specular = specular

  def SetTransmit(self, transmit):
      self.transmit = transmit

  def GetTexturePOVCode(self):
      povCode = "texture { \n"
      if self.transmit < 1:
       povCode += "  pigment { color %s transmit %s }\n" % (self.color,self.transmit)
      else:
       povCode += "  pigment { color %s }\n" % self.color 
      phongCode = ""
      specCode = ""
      if self.phong > 0: 
         phongCode = " phong %f " % self.phong
      if self.specular > 0:
         specCode = " specular %f " % self.specular
      if phongCode != "" or specCode != "": povCode += (" finish { %s %s } " % (phongCode, specCode))
      povCode += "}\n"  
      return povCode 

  def GetCoordPOVCode(self, pos):
      return "<%f, %f, %f>" % (pos[0], pos[1], pos[2])

#
#
#
class Camera(Object):

  def __init__(self, location, direction,up="1*y",camera="perspective",angle="-1",aspect="-1.3333",sky="(0,1,0)",rotatepos="(0,0,0)",rotatevec="(0,0,1)",rotatefac="0.1"):
      self.location = location
      self.direction = direction
      self.aspect = aspect
      self.camera = camera
      self.angle = angle
      self.up = up
      self.sky = sky
      self.rotatepos = rotatepos
      self.rotatevec = rotatevec
      self.rotatefac = rotatefac
  
  def GetPOVCode(self):
      povCode = "camera { \n"
      povCode += "  %s \n" % self.camera
      povCode += "  location %s\n" % self.GetCoordPOVCode(self.location)
      if(self.camera!="orthographic"):
       povCode += "  up 1*y\n"
       povCode += "  right %s*x\n" % self.aspect
       povCode += "  sky %s\n" % self.GetCoordPOVCode(self.sky)
      if(self.camera=="orthographic"):
       if (float(self.angle)!=-1):
        povCode += "  angle %f\n" % float(self.angle)
        povCode += "  right %s*x\n" % self.aspect
       povCode += "  sky %s\n" % self.GetCoordPOVCode(self.sky)
      povCode += "  look_at %s\n" % self.GetCoordPOVCode(self.direction)
      if(self.rotatepos!="(0,0,0)"):
       vector = eval(str(self.rotatevec))
       lst = [0,0,0]
       if(vector[0]!=0): lst[0] = vector[0] + float(self.rotatefac)
       if(vector[1]!=0): lst[1] = vector[1] + float(self.rotatefac)
       if(vector[2]!=0): lst[2] = vector[2] + float(self.rotatefac)
       vct=(lst[0],lst[1],lst[2])
       povCode += "  Rotate_Around_Trans(%s,%s)" % (self.GetCoordPOVCode(vct),self.GetCoordPOVCode(self.rotatepos))
      povCode += "}\n\n"
      return povCode 

#
#
#
class LightSource(Object):

  def __init__(self, location=(0.0, 0.0, 0.0), color="White", shadowless=False, spot=False, point=None, rotatepos="(0,0,0)",rotatevec="(0,0,1)",rotatefac="0.1"):
      self.location = location
      self.color = color
      self.shadowless = shadowless
      self.spotlight = spot
      self.pointto = point
      self.rotatepos = rotatepos
      self.rotatevec = rotatevec
      self.rotatefac = rotatefac

  def SetShadowless(self, value):
      self.shadowless = value

  def GetPOVCode(self):
      shadow = ""
      spt = ""
      if self.shadowless: shadow = "shadowless"
      if self.spotlight: spt = "spotlight point_at <%f %f %f>" % (self.pointto[0], self.pointto[1], self.pointto[2])
      povCode = "light_source { %s color %s %s %s \n" % (self.GetCoordPOVCode(self.location), self.color, shadow, spt)
      if(self.rotatepos!="(0,0,0)"):
       vector = eval(str(self.rotatevec))
       lst = [0,0,0]
       if(vector[0]!=0): lst[0] = vector[0] + float(self.rotatefac)
       if(vector[1]!=0): lst[1] = vector[1] + float(self.rotatefac)
       if(vector[2]!=0): lst[2] = vector[2] + float(self.rotatefac)
       vct=(lst[0],lst[1],lst[2])
       povCode += "  Rotate_Around_Trans(%s,%s)" % (self.GetCoordPOVCode(vct),self.GetCoordPOVCode(self.rotatepos))
      povCode += " } \n"
      return povCode 

#
#
#

class Sphere(Object):

  def __init__(self, pos, radius):
      Object.__init__(self)
      self.pos = pos
      self.radius = radius

  def GetPOVCode(self):
      povCode = "sphere { \n"
      povCode += "     %s, %f\n" % (self.GetCoordPOVCode(self.pos), self.radius) 
      povCode += self.GetTexturePOVCode()
      povCode += "}\n\n"
      return povCode

#
#
#
class BoxPlane(Object):

  def __init__(self, points):
      Object.__init__(self)
      self.points = points

  def GetPOVCode(self):
      povCode = "polygon { \n"
      povCode += "  4, %s, %s, %s, %s\n" % (self.GetCoordPOVCode(self.points[0]),self.GetCoordPOVCode(self.points[1]),self.GetCoordPOVCode(self.points[2]),self.GetCoordPOVCode(self.points[3]))
      povCode += self.GetTexturePOVCode()
      povCode += "}\n\n"
      return povCode
#
#
#

class Box(Object):
      
  def __init__(self, firstcorner, secondcorner):
      Object.__init__(self)
      self.corner1 = firstcorner
      self.corner2 = secondcorner

  def GetPOVCode(self):
      povCode = "box { \n"
      povCode += "     %s, %s\n" % (self.GetCoordPOVCode(self.corner1), self.GetCoordPOVCode(self.corner2)) 
      povCode += self.GetTexturePOVCode()
      povCode += "}\n\n"
      return povCode 

class Cube(Object):
  
  def __init__(self, a=(0,0,0), b=(0,0,0), c=(0,0,0), r=0.1,color=(0.0,0.0,0.0)):
      Object.__init__(self)
      self.va = a
      self.vb = b
      self.vc = c
      self.radius = r

  def GetPOVCode(self):
      moda = sqrt(self.va[0]**2+self.va[1]**2+self.va[2]**2)
      modb = sqrt(self.vb[0]**2+self.vb[1]**2+self.vb[2]**2)
      modc = sqrt(self.vc[0]**2+self.vc[1]**2+self.vc[2]**2)
      vab = (self.va[0]+self.vb[0],self.va[1]+self.vb[1],self.va[2]+self.vb[2])
      vac = (self.va[0]+self.vc[0],self.va[1]+self.vc[1],self.va[2]+self.vc[2])
      vbc = (self.vc[0]+self.vb[0],self.vc[1]+self.vb[1],self.vc[2]+self.vb[2])
      vabc = (self.va[0]+self.vb[0]+self.vc[0],self.va[1]+self.vb[1]+self.vc[1],self.va[2]+self.vb[2]+self.vc[2])
      povCode = "union { \n"
#spheres
      povCode += " sphere{ " + "<0,0,0>"     + ", " + str(self.radius) + " }\n"
      povCode += " sphere{ " + str(self.GetCoordPOVCode(self.va)) + ", " + str(self.radius) + " }\n"
      povCode += " sphere{ " + str(self.GetCoordPOVCode(self.vb)) + ", " + str(self.radius) + " }\n"
      povCode += " sphere{ " + str(self.GetCoordPOVCode(self.vc)) + ", " + str(self.radius) + " }\n"
      povCode += " sphere{ " + str(self.GetCoordPOVCode(vab)) + ", " + str(self.radius) + " }\n"
      povCode += " sphere{ " + str(self.GetCoordPOVCode(vac)) + ", " + str(self.radius) + " }\n"
      povCode += " sphere{ " + str(self.GetCoordPOVCode(vbc)) + ", " + str(self.radius) + " }\n"
      povCode += " sphere{ " + str(self.GetCoordPOVCode(vabc)) + ", " + str(self.radius) + " }\n"
#4cylinders in x
      povCode += " cylinder{ " + "<0,0,0>," + str(self.GetCoordPOVCode(self.va)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(self.vc)) + "," + str(self.GetCoordPOVCode(vac)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(self.vb)) + "," + str(self.GetCoordPOVCode(vab)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(vbc)) + "," + str(self.GetCoordPOVCode(vabc)) + "," + str(self.radius) + " }\n"
#4cylinders in y
      povCode += " cylinder{ " + "<0,0,0>," + str(self.GetCoordPOVCode(self.vb)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(self.va)) + "," + str(self.GetCoordPOVCode(vab)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(self.vc)) + "," + str(self.GetCoordPOVCode(vbc)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(vac)) + "," + str(self.GetCoordPOVCode(vabc)) + "," + str(self.radius) + " }\n"
#4cylinder in z
      povCode += " cylinder{ " + "<0,0,0>," + str(self.GetCoordPOVCode(self.vc)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(self.va)) + "," + str(self.GetCoordPOVCode(vac)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(self.vb)) + "," + str(self.GetCoordPOVCode(vbc)) + "," + str(self.radius) + " }\n"
      povCode += " cylinder{ " + str(self.GetCoordPOVCode(vab)) + "," + str(self.GetCoordPOVCode(vabc)) + "," + str(self.radius) + " }\n" 
#texture
      povCode += self.GetTexturePOVCode()
      povCode += "}\n\n"
      return povCode
#
#
#

class Scene:
      
  def __init__(self):
      self.ambientLight = False
      self.objects = []
      self.includes = ["colors", "stones", "textures", "shapes", "glass", "metals", "woods", "transforms"]
      self.backcolor = "Black"
      self.lights = []

  def Add(self, object):
      self.objects.append(object)

  def AddLight(self, light):
      self.lights.append(light)

  def SetAmbientLight(self, l): self.ambientLight = l

  def SetBackgroundColor(self, color): self.backcolor = color

  def SetLightColor(self, color): self.lights[0].color = color

  def SetLightLocation(self, location): self.lights[0].location = location

  def GeneratePOVCode(self, author="Python Model module"):
      self.povCode = "\n// Generated by %s \n" % author

      for inc in self.includes:
          self.povCode += "\n#include \"%s.inc\"" % inc
      self.povCode += "\n"
      if self.ambientLight: self.povCode += "\nglobal_settings { ambient_light rgb<1, 1, 1> }\n"
      self.povCode += "\nbackground { color %s }\n" % self.backcolor 

      for obj in self.objects:
          self.povCode += "\n\n" + obj.GetPOVCode()
      for light in self.lights:
          self.povCode += "\n"
          self.povCode += light.GetPOVCode()
          self.povCode += "\n"

  def ShowPOV(self):
      self.GeneratePOVCode()
      print self.povCode

  def ExportPOV(self, povfile):
      self.GeneratePOVCode()
      f = open(povfile, 'w')
      f.write(self.povCode)
      f.close()

  def Render(self, outfile, inpfile, op , format="ppm", display=False, size=(320, 240), antialias=False):
      fcode = ""
      if format == "ppm": fcode = "+FP"
      elif format == "png": fcode = "+FN"
      elif format == "tga": fcode = "+FT"
      elif format == "ctga": fcode = "+FC"
      else: fcode = ""
      command = "povray %s +I%s +O%s " % (fcode, inpfile, outfile)
      command += " +W%d +H%d " % (size[0], size[1])
      if antialias: command += "Antialias=%s " % op['antialias']['value']
      if display: command += "Display=True 2> /dev/null "
      else: command += "Display=False 2> /dev/null"
      print ("Executing : %s" , command)
      os.system(command)
      if ("logo" in op):
       command = "convert %s -fill black -stroke black -pointsize 20 -gravity %s -annotate 0 '%s' tmp.png" % (outfile, op['logo']['type'], op['logo']['expression='])
       os.system(command)
       command = "mv tmp.png %s" % outfile
       os.system(command)
#  os.system("rm modeltmp.pov")
