import subprocess
import hashlib
from sys import argv

files_count = argv[1]

for i in range(1, int(files_count) + 1):
	with open(f"{i}_chunk_input", 'r') as input_file:
		data = input_file.readlines()
		for index, d in enumerate(data):
			d = d[0:-1] if d[-1] == '\n' else d
			bd = d.encode('utf-8')
			correct_answer = hashlib.sha256(bd).hexdigest()
			my_hash_output = subprocess.getoutput(f'./main.exe <<< {d}')
			if my_hash_output == correct_answer:
				print(f'line: {index+1}, file: {i}_chunk_input Test case paased')
			else:
				print(f'line: {index+1}, file: {i}_chunk_input Test case Failed')