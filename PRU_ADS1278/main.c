//  pru0adc.c
//  Attempt to duplicate Derek Molloy's
//  SPI ADC read program in C from assembly.
//  Chip Select:  P9.27 pr1_pru0_pru_r30_5
//  MOSI:         P9.29 pr1_pru0_pru_r30_1
//  MISO:         P9.28 pr1_pru0_pru_r31_3
//  SPI CLK:      P9.30 pr1_pru0_pru_r30_2
//  Sample Clock: P8.46 pr1_pru1_pru_r30_1  (testing only)
//  Copyright (C) 2016  Gregory Raven
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "resource_table_0.h"
#include <am335x/pru_cfg.h>
#include <am335x/pru_intc.h>
#include <pru_rpmsg.h>
#include <rsc_types.h>
#include <stdint.h>
#include <stdio.h>



//PRU0
#define DATA_IN     ( (uint8_t)( __R31 & 0x000000FF ) )  //bit 7-0
#define nDRDY       ( __R31 & 0x00004000 )  //bit 14
#define SCLK        ( __R30 & 0x00008000 )  //bit 15
#define PPS        ( __R31 & 0x00010000 )  //bit 15

#define SCLK_SET    ( __R30 =  __R30 | 0x00008000 )  //set bit 15
#define SCLK_CLR    ( __R30 =  __R30 & 0xFFFF7FFF )  //clear bit 15
#define SCLK_TOGGLE ( __R30 =  __R30 ^ 0x00008000 )  //toggle bit 15


//PRU1
//#define nDRDY       ( (uint8_t)( __R31 & 0x00000001 ) )  //bit 0
//#define SCLK        ( (uint8_t)( __R31 & 0x00000002 ) )  //bit 1


// Define remoteproc related variables.
#define HOST_INT ((uint32_t)1 << 30)

//  The PRU-ICSS system events used for RPMsg are defined in the Linux device
//  tree.
//  PRU0 uses system event 16 (to ARM) and 17 (from ARM)
//  PRU1 uses system event 18 (to ARM) and 19 (from ARM)
#define TO_ARM_HOST 16
#define FROM_ARM_HOST 17

//  Using the name 'rpmsg-pru' will probe the rpmsg_pru driver found
//  at linux-x.y.x/drivers/rpmsg_pru.c
#define CHAN_NAME "rpmsg-pru"
#define CHAN_DESC "Channel 30"
#define CHAN_PORT 30
#define PULSEWIDTH 300

//  Used to make sure the Linux drivers are ready for RPMsg communication
//  Found at linux-x.y.z/include/uapi/linux/virtio_config.h
#define VIRTIO_CONFIG_S_DRIVER_OK 4

//  Buffer used for PRU to ARM communication.
uint8_t payload[256];

int8_t test[2];

#define PRU_SHAREDMEM 0x00010000
volatile register uint32_t __R30;
volatile register uint32_t __R31;

int lastPPsState = 0;

int main(void) {
  struct pru_rpmsg_transport transport;
  uint16_t src, dst, len;
  volatile uint8_t *status;

  //  1.  Enable OCP Master Port
  CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;
  //  Clear the status of PRU-ICSS system event that the ARM will use to 'kick'
  //  us.
  CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;

  //  Make sure the drivers are ready for RPMsg communication:
  status = &resourceTable.rpmsg_vdev.status;
  while (!(*status & VIRTIO_CONFIG_S_DRIVER_OK))
    ;

  //  Initialize pru_virtqueue corresponding to vring0 (PRU to ARM Host
  //  direction).
  pru_rpmsg_init(&transport, &resourceTable.rpmsg_vring0,
                 &resourceTable.rpmsg_vring1, TO_ARM_HOST, FROM_ARM_HOST);

  // Create the RPMsg channel between the PRU and the ARM user space using the
  // transport structure.
  while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_NAME, CHAN_DESC,
                           CHAN_PORT) != PRU_RPMSG_SUCCESS)
    ;
  //  The above code should cause an RPMsg character to device to appear in the
  //  directory /dev.
  //  The following is a test loop.  Comment this out for normal operation.

  //  This section of code blocks until a message is received from ARM.
  while (pru_rpmsg_receive(&transport, &src, &dst, payload, &len) !=
         PRU_RPMSG_SUCCESS) {
  }
  /* Priming the 'hostbuffer' with a message */
  uint8_t hello[] = "hello \n";
  pru_rpmsg_send(&transport, dst, src, hello, 10); //buffer length check
  SCLK_CLR;
  uint32_t cCount = 0;

//
 //initCcount();
 enableCcount();
  __delay_cycles(1000);
 cCount = getCcount();

  payload[0] = 0x41+ (uint8_t)((cCount & 0xFF000000)>>24);
  payload[1] = 0x41+ (uint8_t)((cCount & 0x00FF0000)>>16);
  payload[2] = 0x41+ (uint8_t)((cCount & 0x0000FF00)>>8);
  payload[3] = 0x41+ (uint8_t)((cCount & 0x000000FF));


  pru_rpmsg_send(&transport, dst, src, payload, 10); //buffer length check

  uint8_t banaan[] = "geel \n";
  pru_rpmsg_send(&transport, dst, src, banaan, 10); //buffer length check

  __delay_cycles(1000);
 cCount = getCcount();

  payload[0] = 0x41+ (uint8_t)((cCount & 0xFF000000)>>24);
  payload[1] = 0x41+ (uint8_t)((cCount & 0x00FF0000)>>16);
  payload[2] = 0x41+ (uint8_t)((cCount & 0x0000FF00)>>8);
  payload[3] = 0x41+ (uint8_t)((cCount & 0x000000FF));


  pru_rpmsg_send(&transport, dst, src, payload, 10); //buffer length check

  uint8_t rood[] = "rood \n";
    pru_rpmsg_send(&transport, dst, src, rood, 10); //buffer length check

    clearCcount();

    cCount = getCcount();

      payload[0] = 0x41+ (uint8_t)((cCount & 0xFF000000)>>24);
      payload[1] = 0x41+ (uint8_t)((cCount & 0x00FF0000)>>16);
      payload[2] = 0x41+ (uint8_t)((cCount & 0x0000FF00)>>8);
      payload[3] = 0x41+ (uint8_t)((cCount & 0x000000FF));


      pru_rpmsg_send(&transport, dst, src, payload, 10); //buffer length check

      uint8_t groen[] = "groen \n";
        pru_rpmsg_send(&transport, dst, src, groen, 10); //buffer length check
//  __delay_cycles(10000);
//
//  cCount = getCcount();
//    payload[0] = (uint8_t)((count & 0xFF000000)>>24);
//    payload[1] = (uint8_t)((count & 0x00FF0000)>>16);
//    payload[2] = (uint8_t)((count & 0x0000FF00)>>8);
//    payload[3] = (uint8_t)((count & 0x000000FF));
//    pru_rpmsg_send(&transport, dst, src, payload, 4); //buffer length check
//

  while (1)
  {

     checkPPS();


     while( nDRDY ){
         checkPPS();
     }
     cCount = getCcount();
//     SCLK_SET; //clock high for first bit
//     payload[0] = DATA_IN; //reed MSB in parallel
//     SCLK_CLR;
//     __delay_cycles(7);

     payload[0] = (uint8_t)((cCount & 0xFF000000)>>24);
     payload[1] = (uint8_t)((cCount & 0x00FF0000)>>16);
     payload[2] = (uint8_t)((cCount & 0x0000FF00)>>8);
     payload[3] = (uint8_t)((cCount & 0x000000FF));

     int i = 0;
     for ( i = 0; i < 24; i++)  //  Inner single sample loop
     {
         //cycle clock
         SCLK_SET; //clock high for first bit

         payload[4+i] = DATA_IN; //only 8 lsb to payload

         SCLK_CLR;
         checkPPS();

     }

  }

  __halt(); // halt the PRU
}

void initCcount(){

   asm volatile ("  LDI32    r0, 0x00022028 \n");
   asm volatile( " LDI32    r1, 0x00022000 \n" );
   asm volatile( " SBBO   &r1, r0, 0, 4 \n" );
   asm volatile( " JMP r3.w2 \n" );
}
void enableCcount(){
  asm volatile( " LDI32    r1, 0x00022000 \n" );
    //asm volatile("   LBCO   &r2, C28, 0, 4  \n");
    asm volatile("  LBBO    &r4, r1, 0, 4 \n" );
    asm volatile( " SET    r4, r4.t3 \n" );
    asm volatile(" SBBO   &r4, r1, 0, 4 \n" );
    asm volatile(" JMP r3.w2 \n" );

}
uint32_t getCcount(){
    asm volatile( " LDI32    r1, 0x00022000 \n" );
    asm volatile ("   LBBO   &r14, r1, 0xC, 4 ");
    asm volatile("    JMP r3.w2 ");
}

void clearCcount(){


  asm volatile( " LDI32    r1, 0x00022000 \n" );
  asm volatile("  LBBO    &r4, r1, 0, 4 \n" );
  asm volatile( " CLR    r4, r4.t3 \n" );
  asm volatile(" SBBO   &r4, r1, 0, 4 \n" );

  asm volatile( " LDI32    r6, 0 \n" );
  asm volatile ("   SBBO   &r6, r1, 0xC, 4 ");

  asm volatile("  LBBO    &r4, r1, 0, 4 \n" );
  asm volatile( " SET    r4, r4.t3 \n" );
  asm volatile(" SBBO   &r4, r1, 0, 4 \n" );
  asm volatile("    JMP r3.w2 ");
}
void checkPPS(){

    if(PPS >0) && lastPPsState == 0{ //rising edge detected
            clearCcount();
    }
    lastPPsState = PPSstate;

}




