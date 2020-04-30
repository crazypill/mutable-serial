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

void ReadPots( int fd );
void ReadCVs( int fd );
void DoCalibrateLoop( int fd );



// commands
enum
{
    kReadPot,
    kReadCV,
    kReadGate,
    kBypass,
    kCalibrate
};


// read args
enum
{
    kPositionPot,
    kDensityPot,
    kSizePot,
    kSizeCV,
    kPitchPot,
    kPitchCV,
    kBlendPot,
    kBlendCV,
    kTexturePot,
    kTextureCV
};

#define kNumPots 6
#define kNumCVs  4

static const int s_pot_indices[] = { kPositionPot, kDensityPot, kSizePot, kPitchPot, kBlendPot, kTexturePot }; 
static const int s_cv_indices[]  = { kSizeCV, kPitchCV, kBlendCV, kTextureCV }; 



// read arg names
static const char* s_adc_names[] = 
{ 
    "Position pot-CV",  // 0
    "Density pot-CV",   // 1
    "Size pot",         // 2
    "Size CV",          // 3
    "Pitch pot",        // 4
    "V/Oct CV",         // 5
    "Blend pot",        // 6
    "Blend CV",         // 7
    "Texture pot",      // 8
    "Texture CV"        // 9
};


void buffer_input_flush()
{
    int c;
     // This will eat up all other characters
    while( (c = getchar()) != EOF && c != '\n' )
        ;
}


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
    
    static int lastChar = '1';
    while( 1 )
    {
        int inputChar = getchar();
        if( inputChar == 'c' )
        {
            DoCalibrateLoop( fd );
            inputChar = 0;
        }
            
        // check for last command command {enter}
        if( inputChar == '\n' )
            inputChar = lastChar;
        else
            buffer_input_flush();
        
        if( inputChar == '1' )
            ReadPots( fd );
        else if( inputChar == '2' )
            ReadCVs( fd );
        
        lastChar = inputChar;
        printf( "\n" );
    }
    return 0;
}


void ReadPots( int fd )
{
    if( fd == -1 )
        return;

    uint8_t txByte = 0;
    uint8_t rxByte = 0;

    for( int i = 0; i < kNumPots; i++ )
    {
        int index = s_pot_indices[i];
        txByte = index; // read command, arg is the ADC we are reading
        ssize_t result = write( fd, &txByte, 1 );
        if( result < 0 )
            printf( "write error[%d]: %ld\n", index, result );
        
        // read result
        result = read( fd, &rxByte, 1 );
        if( result < 0 )
            printf( "read error[%d]: %s - %ld\n", index, s_adc_names[index], result );
        else
            printf( "%s: %d\n", s_adc_names[index], rxByte );
    }
}


void ReadCVs( int fd )
{
    if( fd == -1 )
        return;
    
    uint8_t txByte = 0;
    uint8_t rxByte = 0;
    
    for( int i = 0; i < kNumCVs; i++ )
    {
        int index = s_cv_indices[i];
        txByte = index; // read command, arg is the ADC we are reading
        ssize_t result = write( fd, &txByte, 1 );
        if( result < 0 )
            printf( "write error[%d]: %ld\n", index, result );
        
        // read result
        result = read( fd, &rxByte, 1 );
        if( result < 0 )
            printf( "read error[%d]: %s - %ld\n", index, s_adc_names[index], result );
        else
            printf( "%s: %d\n", s_adc_names[index], rxByte );
    }
}


void DoCalibrateLoop( int fd )
{
    printf( "Entering calibration mode\n" );
    
    uint8_t txByte = kCalibrate << 5;
    ssize_t result = write( fd, &txByte, 1 );
    if( result < 0 )
        printf( "DoCalibrateLoop: write error: %ld\n", result );
    
    printf( "Hold a C2 note (1V) to v/oct input, then press enter...\n" );
    getchar();
    buffer_input_flush();
    
    // put device into C1 calibration mode...
    txByte = (kCalibrate << 5) | 1;
    result = write( fd, &txByte, 1 );
    if( result < 0 )
        printf( "DoCalibrateLoop: write error: %ld\n", result );
    else
        printf( "DoCalibrateLoop: success!\n" );
    

    printf( "Hold a C4 note (3V) to v/oct input, then press enter...\n" );
    getchar();
    buffer_input_flush();
    
    // put device into C3 calibration mode...
    txByte = (kCalibrate << 5) | 2;
    result = write( fd, &txByte, 1 );
    if( result < 0 )
        printf( "DoCalibrateLoop: write error: %ld\n", result );
    else
        printf( "DoCalibrateLoop: success!\n" );
}
