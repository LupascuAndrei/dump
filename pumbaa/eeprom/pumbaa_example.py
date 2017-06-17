'''
OUTPUT for the script below : 
>>> from drivers import Eeprom
>>> x = Eeprom(19,22,8000)
>>> aux = 'test string, please no buggy bug. Note that this string length is bigger than the eeprom page size, so we will be writing more pages in the same call'
>>> len(aux)
149
>>> x.write(0,0,aux) # device 0, address 0 
>>> x.read(0,0,170) #device 0, address 0, 170 bytes 
b'test string, please no buggy bug. Note that this string length is bigger than the eeprom page size, so we will be writing more pages in the same
 call\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff'
>>> x.read(0, 40, 100)
b'hat this string length is bigger than the eeprom page size, so we will be writing more pages in the '
>>> aux[40:140]
'hat this string length is bigger than the eeprom page size, so we will be writing more pages in the '

'''

from drivers import Eeprom
x = Eeprom(19,22,8000)
aux = 'test string, please no buggy bug. Note that this string length is bigger than the eeprom page size, so we will be writing more pages in the same call'
print(len(aux))
print(x.write(0,0,aux)) # device 0, address 0 
print(x.read(0,0,170)) #device 0, address 0, 170 bytes 
print(x.read(0, 40, 100))
print(aux[40:140])
