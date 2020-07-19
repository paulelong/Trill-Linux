#pragma once

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_ESP32
#include <driver/i2c.h>
#define MAX_I2C_TX_BUFFER_LENGTH		0
#define MAX_I2C_RX_BUFFER_LENGTH		0
#define CONFIG_I2C_SLAVE_SCL			22
#define CONFIG_I2C_SLAVE_SDA			23
#define CONFIG_I2C_MASTER_FREQUENCY		400000
#define TICKS_TO_WAIT					1000 / portTICK_RATE_MS //TODO from example, how many ticks to wait??
#define ACK_CHECK_EN i2c_ack_type_t(0x1)                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS i2c_ack_type_t(0x0)                       /*!< I2C master will not check ack from slave */

#define ACK_VAL i2c_ack_type_t(0x0)                             /*!< I2C ack value */
#define NACK_VAL i2c_ack_type_t( 0x1)                            /*!< I2C nack value */

#else
#include <linux/i2c-dev.h>
// heuristic to guess what version of i2c-dev.h we have:
// the one installed with `apt-get install libi2c-dev`
// would conflict with linux/i2c.h, while the stock
// one requires linus/i2c.h
#ifndef I2C_SMBUS_BLOCK_MAX
// If this is not defined, we have the "stock" i2c-dev.h
// so we include linux/i2c.h
#include <linux/i2c.h>
typedef unsigned char i2c_char_t;
#else
typedef char i2c_char_t;
#endif
#endif

#include <sys/ioctl.h>

#define MAX_BUF_NAME 64

class I2c
{

protected:
	int i2C_bus;
	int i2C_address;
	int i2C_file;

public:
	int initI2C_RW(int bus, int address, int file);
	virtual int readI2C() = 0;
	int rawread(uint8_t* buf, size_t length);
	int rawwrite(char* buf, uint8_t length);
	int closeI2C();

	virtual ~I2c();

};

inline int I2c::rawread(uint8_t* buf, size_t length)
{
#ifdef USE_ESP32
	int ret;
	// return i2c_master_write_buffer(i2C_bus, buf, length, TICKS_TO_WAIT);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if((ret = i2c_master_start(cmd)))
	{
		printf("read i2c_master_start error %d\n", ret);
	}

	ret = i2c_master_write_byte(cmd, (i2C_address << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    if(ret)
	{
		printf("read i2c_master_write_byte address error %d\n", ret);
	}

    if (length > 1) {
        i2c_master_read(cmd, buf, length - 1, ACK_VAL);
    }

	ret = i2c_master_read_byte(cmd, buf + length - 1, NACK_VAL);
    if(ret)
	{
		printf("read i2c_master_read_byte data error %d\n", ret);
	}

    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2C_bus, cmd, 1000 / portTICK_RATE_MS);
    if(ret)
	{
		printf("rawread i2c_master_cmd_begin error %d\n", ret);
	}

	i2c_cmd_link_delete(cmd);

	return ret ? ret : length;
#else
	return ::read(i2C_file, buf, length);
#endif
}

inline int I2c::rawwrite(char* buf, uint8_t length)
{
#ifdef USE_ESP32
	int ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if((ret = i2c_master_start(cmd)))
	{
		printf("write i2c_master_start error %d\n", ret);
	}
	
	ret = i2c_master_write_byte(cmd, (i2C_address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    if(ret)
	{
		printf("write i2c_master_read_byte address error %d\n", ret);
	}

	ret = i2c_master_write(cmd, (uint8_t*)buf, length, ACK_CHECK_EN);
	if(ret)
	{
		printf("write i2c_master_read_byte data error %d\n", ret);
	}

    if((ret = i2c_master_stop(cmd)))
	{
		printf("i2c_master_stop error %d\n", ret);
	}
	
	ret = i2c_master_cmd_begin(i2C_bus, cmd, 1000 / portTICK_RATE_MS);
	if(ret)
	{
		printf("rawwrite error %d\n", ret);
	}
    i2c_cmd_link_delete(cmd);

	return ret ? ret : length;
#else
	return ::write(i2C_file, buf, length);
#endif
}

inline int I2c::initI2C_RW(int bus, int address, int fileHnd)
{
	i2C_bus 	= bus;
	i2C_address = address;
	i2C_file 	= fileHnd;

#ifdef USE_ESP32
    i2c_config_t conf;
    conf.sda_io_num = CONFIG_I2C_SLAVE_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = CONFIG_I2C_SLAVE_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.mode = I2C_MODE_MASTER;
    //conf.slave.slave_addr = i2C_address;
	conf.master.clk_speed = CONFIG_I2C_MASTER_FREQUENCY;

	int ret;
    ret = i2c_driver_install(i2C_bus, conf.mode, MAX_I2C_RX_BUFFER_LENGTH, MAX_I2C_TX_BUFFER_LENGTH, 0);
    if(ret)
	{
		printf("Install failed %d\n", ret);
	}

	ret = i2c_param_config(i2C_bus, &conf);
    if(ret)
	{
		printf("config failed %d\n", ret);
	}

	return ret;
#else
	open I2C device as a file
	char namebuf[MAX_BUF_NAME];
	snprintf(namebuf, sizeof(namebuf), "/dev/i2c-%d", i2C_bus);

	if ((i2C_file = open(namebuf, O_RDWR)) < 0)
	{
		fprintf(stderr, "Failed to open %s I2C Bus\n", namebuf);
		return(1);
	}

	// target device as slave
	if (ioctl(i2C_file, I2C_SLAVE, i2C_address) < 0){
		fprintf(stderr, "I2C_SLAVE address %#x failed...", i2C_address);
		return(2);
	}

	return 0;
#endif
}



inline int I2c::closeI2C()
{
#ifdef USE_ESP32

	return i2c_driver_delete(i2C_bus);

#else
	if(close(i2C_file)>0)
	{
		fprintf(stderr, "Failed to close  file %d\n", i2C_file);
		return 1;
	}
	return 0;
#endif
}


inline I2c::~I2c(){}
