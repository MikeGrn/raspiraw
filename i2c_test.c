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

void read_register(int fd, uint8_t addr, uint16_t reg) {

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

	ssize_t r;
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

	//for (uint16_t i = 0x3000; i < 0x3200; i++) 
	{
		uint8_t i2c_addr = 0x10;
		//reg = 0x30B4;
		// reg = 0x30b6; // temp calibration
		// reg = 0x30b2; // temp value
		// reg = 0x30b4; // temp controller
		// reg = 0x30b0; // stream
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
	}
	// r = command(0x30B4, file);
	// printf("r: %d", r);
	// if (r < 0) {
	// 	perror("Error");
	// 	printf("Error %d\n", r);
	// 	goto close_file;
	// }

	// r = read(file, &buf, 2);
	// if (r < 0) {
	// 	perror("Error");
	// 	printf("Error %d\n", r);
	// 	goto close_file;
	// }
	// printf("%s\n", buf);
	// printf("%x%x %d\n", buf[0] & 0xFF, buf[1] & 0xFF, r);

close_file:
	close(file);
}
