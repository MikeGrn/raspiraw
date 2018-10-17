#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

ssize_t command(uint16_t reg, int fd) {
    printf("Writing to %d\n", fd);

    unsigned char reg_buf[2];
    ssize_t w = 0;
    ssize_t size = sizeof(unsigned char)*2;

    reg_buf[1] = (reg >> 0) & 0xFF;
    reg_buf[0] = (reg >> 8) & 0xFF;

    w = write(fd, reg_buf, size);
    return w;
}

void reg_to_buff(uint16_t reg, unsigned char *buf) {
	buf[0] = (reg >> 8) & 0xFF;
	buf[1] = (reg >> 0) & 0xFF;
}

int
i2c_write16(int i2c_fd, int addr,uint16_t reg, uint16_t value) {
	struct i2c_rdwr_ioctl_data msgset;
	struct i2c_msg iomsgs[1];
	char buf[4];
	int rc;

	reg_to_buff(reg, buf);
	buf[2] = (char) (( value >> 8 ) & 0xFF);
	buf[3] = (char) (value & 0xFF);

	iomsgs[0].addr = (unsigned) addr;
	iomsgs[0].flags = 0;		/* Write */
	iomsgs[0].buf = buf;
	iomsgs[0].len = 4;

	msgset.msgs = iomsgs;
	msgset.nmsgs = 1;

	rc = ioctl(i2c_fd,I2C_RDWR,&msgset);
	return rc < 0 ? -1 : 0;
}

// uint8_t i2c_addr = 0x10;
// reg = 0x30b6; // temp calibration
// reg = 0x30B4;
// reg = 0x30b2; // temp value
// reg = 0x30b4; // temp controller
// reg = 0x301a; // stream
ssize_t read_register(int fd, uint8_t i2c_addr, uint16_t reg, uint16_t *value) {

		unsigned char out_buf[4];
		uint8_t buf[2];
		reg_to_buff(reg, buf);
		struct i2c_rdwr_ioctl_data msgset;
		struct i2c_msg msgs[2] = {
			{
				 .addr = i2c_addr,
				 .flags = 0,
				 .len = 2,
				 .buf = buf,
			},
			{
				.addr = i2c_addr,
				.flags = I2C_M_RD,
				.len = 2,
				.buf = out_buf,
			},
		};

		msgset.msgs = msgs;
		msgset.nmsgs = 2;
		ssize_t r = ioctl(fd, I2C_RDWR, &msgset);
		*value = out_buf[0] << 8 | out_buf[1];
		return r;
}

void show_register(int fd, uint8_t i2c_addr, uint16_t reg) {
	uint16_t value;
	ssize_t r = read_register(fd, i2c_addr, reg, &value);
	if (r) {
		printf("%04X=%04X\n", reg, value);
	} else {
		printf("Failed\n");
	}
}

void write_register(int fd, uint8_t i2c_addr, uint16_t reg, uint16_t value) {
	unsigned char out_buf[4];
	reg_to_buff(reg, out_buf);
	reg_to_buff(value, &out_buf[2]);
	ssize_t r = write(fd, out_buf, 4);
	if (r != 4) {
		printf("Failed\n");
	}
}

int main(int argc, char **argv) {
	if (argc == 1) {
		printf("Options: show_streaming|start_streaming|stop_streaming\n");
		return -1;
	}

	int file;
	char *filename = "/dev/i2c-0";
	if ((file = open(filename, O_RDWR)) < 0) {
		perror("Failed to open i2c bus");
		exit(1);
	}

	int addr = 0x10;          // The I2C address of the ADC
	if (ioctl(file, I2C_SLAVE, addr) < 0) {
	    printf("Failed to acquire bus access and/or talk to slave.\n");
	    /* ERROR HANDLING; you can check errno to see what went wrong */
	    goto close_file;
	}

	if (strcmp(argv[1], "show_streaming") == 0) {
		show_register(file, 0x10, 0x301a);
	} else if (strcmp(argv[1], "start_streaming") == 0) {
		show_register(file, 0x10, 0x301a);
		write_register(file, 0x10, 0x301a, 0x205C);
		show_register(file, 0x10, 0x301a);
	} else if (strcmp(argv[1], "stop_streaming") == 0) {
		show_register(file, 0x10, 0x301a);
		write_register(file, 0x10, 0x301a, 0x2058);
		show_register(file, 0x10, 0x301a);
	}

int r;
	//printf("Temp value: ");
	//read_register(file, 0x10, 0x30b2);
	//read_register(file, 0x10, 0x30b4);

	//r = i2c_write16(file, 0x10, 0x30b4, 0b10001);

	//printf("Temp value: ");
	//read_register(file, 0x10, 0x30b2);
	//read_register(file, 0x10, 0x30b4);

	//r = i2c_write16(file, 0x10, 0x30b4, 0);
	//printf("Temp value: ");
	//read_register(file, 0x10, 0x30b2);
	//read_register(file, 0x10, 0x30b4);
	//read_register(file, 0x10, 0x30b2);


/*	printf("Streaming: ");
	read_register(file, 0x10, 0x301a);

	unsigned char out_buf[4];
	reg_to_buff(0x301a, out_buf);
	out_buf[3] = 0b01011100;
	out_buf[2] = 0b100000;
	r = write(file, out_buf, 4);

	printf("Streaming: ");
	read_register(file, 0x10, 0x301a);*/


close_file:
	close(file);
}
