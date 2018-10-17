#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

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
void read_register(int fd, uint8_t i2c_addr, uint16_t reg) {

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
		printf("%d: %02X%02X=%02X%02x\n", r, msgs[0].buf[0], msgs[0].buf[1], out_buf[0], out_buf[1]);
}

void main() {
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


	printf("Streaming: ");
	read_register(file, 0x10, 0x301a);

	unsigned char out_buf[4];
	reg_to_buff(0x301a, out_buf);
	out_buf[3] = 0b01011100;
	out_buf[2] = 0b100000;
	r = write(file, out_buf, 4);

	printf("Streaming: ");
	read_register(file, 0x10, 0x301a);

/*	ssize_t r;
	uint16_t i = 0x301a;
	r = command(i, file);
	uint16_t reg = i;
	unsigned char out_buf[4];
	reg_to_buff(reg, out_buf);
	out_buf[3] = 0b01011100;
	out_buf[2] = 0b100000;
	r = write(file, out_buf, 4);
	if (r != 4) {
		perror("Error");
		goto close_file;
	}
*/
		/*uint8_t i2c_addr = 0x10;
		//reg = 0x30B4;
		// reg = 0x30b6; // temp calibration
		// reg = 0x30b2; // temp value
		// reg = 0x30b4; // temp controller
		// reg = 0x301a; // stream
		reg = 0x301a;
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
		r = ioctl(file, I2C_RDWR, &msgset);
		//printf("%d\n", r);
		//printf("Read i2c addr %02X, reg %02X%02X (len %d), value %02X %02X, err %d\n", i2c_addr, msgs[0].buf[1], msgs[0].buf[0], msgs[0].len, out_buf[0], out_buf[1], r);
		
		printf("%02X%02X=%02X%02x\n", msgs[0].buf[0], msgs[0].buf[1], out_buf[0], out_buf[1]);
		*/

close_file:
	close(file);
}
