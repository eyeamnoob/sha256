from string import ascii_lowercase, ascii_uppercase, digits
from random import choices
from sys import argv

INPUT_PER_CHUNK = 64
chunks_count = argv[1]

characters = ascii_lowercase + ascii_uppercase + digits

for i in range(1, int(chunks_count) + 1):
	with open(f"{i}_chunk_input", "w") as input_file:
		lines = list()
		for j in range(max(1, INPUT_PER_CHUNK * (i - 1) - 8), INPUT_PER_CHUNK * i - 8):
			random_input = choices(characters, k=j)
			lines.append(''.join(random_input))

		input_file.write('\n'.join(lines))
