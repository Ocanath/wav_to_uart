import serial
from serial.tools import list_ports


def autoconnect_serial():
	""" 
		Find a serial com port.
	"""
	com_ports_list = list(list_ports.comports())
	port = []
	slist = []
	for p in com_ports_list:
		if(p):
			pstr = ""
			pstr = p
			port.append(pstr)
			print("Found:", pstr)
	if not port:
		print("No port found")

	for p in port:
		try:
			ser = []
			ser = (serial.Serial(p[0],'2000000', timeout = 0))
			slist.append(ser)
			print ("connected!", p)
			break
			# print ("found: ", p)
		except:
			print("failded.")
			pass
	print( "found ", len(slist), "ports.")
	return slist
