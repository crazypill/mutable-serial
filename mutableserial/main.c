//
//  main.c
//  MutableSerial
//
//  Created by Alex Lelievre on 4/28/20.
//  Copyright © 2020 Far Out Labs. All rights reserved.
//

#include <stdio.h>
#include <stdbool.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <unistd.h>


#define PORT_ERROR -1


int main(int argc, const char * argv[]) {
    
    // open the /dev/cu.SLAB_USBtoUART
    
    bool blocking = false;

    //Settings structure old and new
    struct termios newtio;
    
    int fd = open( "/dev/cu.SLAB_USBtoUART", O_RDWR | O_NOCTTY | (blocking ? 0 : O_NDELAY) );
    if( fd < 0 )
    {
        return PORT_ERROR;
    }

    bzero( &newtio, sizeof( newtio ) );
    
    if( cfsetispeed( &newtio, B9600 ) != 0 )
        return PORT_ERROR;
    if( cfsetospeed( &newtio, B9600 ) != 0 )
        return PORT_ERROR;
    
    newtio.c_cflag &= ~PARENB;
    newtio.c_cflag &= ~CSTOPB;
    newtio.c_cflag &= ~CSIZE;
    newtio.c_cflag |= CS8;
    newtio.c_cflag &= ~CRTSCTS;
    
    //Hardware control of port
    newtio.c_cc[VTIME] = blocking ? 1 : 0; // Read-timout 100ms when blocking
    newtio.c_cc[VMIN] = 0;
    
    tcflush( fd, TCIFLUSH );
    
    // Acquire new port settings
    if( tcsetattr( fd, TCSANOW, &newtio ) != 0 )
        puts( strerror( errno ) );

    // write a read command
    if( fd == -1 )
        return PORT_ERROR;
    
    uint8_t txByte = 0;
    uint8_t rxByte = 0;
    
    while( 1 )
    {
        int toggle = 0;
//        int i = 0;
        for( int i = 0; i < 10; i++ )
        {
//            txByte = (3 << 5) | toggle;//i;
            txByte = i; // read command, arg is the ADC we are reading
            ssize_t result = write( fd, &txByte, 1 );
            if( result < 0 )
                printf( "write error[%d]: %ld\n", i, result );
            
            // read result
            result = read( fd, &rxByte, 1 );
            if( result < 0 )
                printf( "read error[%d]: %ld\n", i, result );
            else
                printf( "read[%d]: %d\n", i, rxByte );
            
            toggle = !toggle;
        }
        
        sleep( 5 );
        printf( "\n" );
    }
    return 0;
}
