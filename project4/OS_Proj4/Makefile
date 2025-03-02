all: fs_sim fs_reader fsck

fs_sim: fs_sim.c fs.c fs.h fs_util.c file.c directory.c disk.c disk.h API.h
		gcc fs_sim.c fs.c disk.c fs_util.c file.c directory.c -g -o fs_sim

fs_reader: fs_reader.c fs.c fs.h fs_util.c file.c directory.c disk.c disk.h API.h
		gcc fs_reader.c fs.c disk.c fs_util.c file.c directory.c -g -o fs_reader

fsck: fsck.c fs.c fs.h fs_util.c file.c directory.c disk.c disk.h API.h
		gcc fsck.c fs.c disk.c fs_util.c file.c directory.c -g -o fsck

clean:
		rm -f fs_sim fs_reader fsck
