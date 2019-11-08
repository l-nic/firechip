# Build a list of id's and ports sufficient to run the othello sim
# Also launch nodes to actually execute the sim
import argparse, Queue, subprocess, shlex, time, os

class Node:
	def __init__(self, own_id, neighbor_ids):
		self.own_id = own_id
		self.neighbor_ids = neighbor_ids

def get_child_id(own_id, i, branch_factor):
	return (branch_factor * own_id) - (branch_factor - 2) + i

def main():
	parser = argparse.ArgumentParser()
	branch_factor = 4 # This is fixed for now
	parser.add_argument("--levels", type=int, help="Levels of the map-reduce tree", default=3)
	parser.add_argument("--base-port", type=int, help="Starting port number for nodes to use", default=9000)
	parser.add_argument("--use-riscv", type=bool, help="Use the riscv simulator instead of the C++ node", default=False)

	args = parser.parse_args()
	use_riscv = args.use_riscv
	node_queue = Queue.Queue()
	node_queue.put(Node(1, []))
	node_list = []

	if use_riscv:
		# Compile the riscv othello C program to assembly
		os.system("riscv64-unknown-elf-gcc -S -fverbose-asm riscv_othello_node.c")

		# Use the custom assembler to assemble to object code
		os.system("/home/vagrant/firechip/lnic-dev/binutils-gdb/build/gas/as-new riscv_othello_node.s -o riscv_othello_node.o")

		# Link the object code into the final binary
		link_incantation = "riscv64-unknown-elf-ld -L/opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0 /opt/riscv/riscv64-unknown-elf/lib/crt0.o /opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0/crtbegin.o riscv_othello_node.o -lgcc -lc -lgloss -lc /opt/riscv/lib/gcc/riscv64-unknown-elf/7.2.0/crtend.o -o riscv_othello_node.riscv"
		os.system(link_incantation)

	# Connect all the nodes and store them in a node set
	for level in range(args.levels):
		next_queue = Queue.Queue()
		while (not node_queue.empty()):
			current_node = node_queue.get()
			# Skip the branching for the last level
			if level < args.levels - 1:
				for i in range(branch_factor):
					child_id = get_child_id(current_node.own_id, i, branch_factor)
					next_queue.put(Node(child_id, [current_node.own_id]))
					current_node.neighbor_ids.append(child_id)
			node_list.append(current_node)
		node_queue = next_queue

	# Write the id-port definitions file
	id_file = open("id_addr.txt", "w")
	node_port = args.base_port
	id_file.write("ID,IP Address,Port\n")
	for node in node_list:
		id_file.write(str(node.own_id) + ",127.0.0.1," + str(node_port) + "\n")
		node_port += 1
	id_file.close()

	# Launch the nodes
	log_files = []
	subprocs = []
	for node in node_list:
		if not use_riscv:
			node_command = "./othello_node " + str(node.own_id)
			for neighbor in node.neighbor_ids:
				node_command += " " + str(neighbor)
		else:
			node_command = "/home/vagrant/firechip/lnic-dev/riscv-isa-sim/build/spike --nic-config-data="
			node_command += str(node.own_id) + ","
			for neighbor in node.neighbor_ids:
				node_command += str(neighbor) + ","
			node_command = node_command[:-1]
			node_command += " /home/vagrant/firechip/lnic-dev/riscv-pk/build/pk riscv_othello_node.riscv "
			node_command += str(node.own_id)
		log_file = open("logs/othello_log_" + str(node.own_id) + ".txt", "w")
		log_files.append(log_file)

		print node_command
		args = shlex.split(node_command)
		p = subprocess.Popen(args, stdout=log_file, stderr=log_file)
		subprocs.append(p)

	time.sleep(10)
	for i in range(len(log_files)):
		log_files[i].close()
		subprocs[i].kill()


if __name__ == "__main__":
	main()