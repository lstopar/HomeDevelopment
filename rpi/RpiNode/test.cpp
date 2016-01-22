#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>


// Written by John Newbigin jnewbigin@chrysocome.net
// Based on info from http://www.kernel.org/doc/Documentation/i2c/dev-interface
// And http://doc.chipfind.ru/pdf/philips/pca8591.pdf

int main( int argc, char **argv )
{
        int i;
        int r;
        int fd;
        unsigned char command[2];
        unsigned char value[4];
        useconds_t delay = 2000;

        char *dev = "/dev/i2c-1";
        int addr = 0x48;

        printf("PCF8591 Test\n");

        fd = open(dev, O_RDWR );
        if(fd < 0)
        {
                perror("Opening i2c device node\n");
                return 1;
        }

        r = ioctl(fd, I2C_SLAVE, addr);
        if(r < 0)
        {
                perror("Selecting i2c device\n");
        }

        while(1)
        {
                for(i = 0; i < 4; i++)
                {
                        command[0] = 0x40 | ((i + 1) & 0x03); // output enable | read input i
                        command[1]++;
                        r = write(fd, &command, 2);
                        usleep(delay);
                        // the read is always one step behind the selected input
                        r = read(fd, &value[i], 1);
                        if(r != 1)
                        {
                                perror("reading i2c device\n");
                        }
                        usleep(delay);
                }
                printf("0x%02x 0x%02x 0x%02x 0x%02x\n", value[0], value[1], value[2], value[3]);
        }

        close(fd);
        return(0);
}
