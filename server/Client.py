import socket
import json

class Client(object):

	def __init__(self, ip, port):
		self.ip   = ip
		self.port = port

		self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	def connect():
		self.socket.connect(self.ip, self.port)

	def read(self, key):
		data = '{"type" : "read", "key" : "' + key + '"}'
		self.socket.send(data)

		buff = self.socket.recv(data)
		return json.loads(buff)

	def write(self, key, value):
		data = '{"type" : "read", "key" : "' + key + '", "value" : "' + value + '"}'
		self.socket.send(data)

		buff = self.socket.recv(data)
		return json.loads(buff)

	def close(self):
		self.socket.close()