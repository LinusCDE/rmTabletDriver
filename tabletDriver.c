/*
 * Does the same as tableDriver.py !
 * Requires linux 4.5 rc1 or higher.
 */

// Custom device:
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdint.h>

// Client connection:
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct input_packet {
	uint16_t type;
	uint16_t code;
	int32_t value;
};

void emit(int fd, int type, int code, int val)
{
   struct input_event ie;

   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   write(fd, &ie, sizeof(ie));
}


void addAbsCapability(int fd, int code, int32_t value, int32_t min, int32_t max, int32_t resolution, int32_t fuzz, int32_t flat) {
   ioctl(fd, UI_SET_ABSBIT, code); // Add capability

   struct input_absinfo abs_info;
   abs_info.value = value;
   abs_info.minimum = min;
   abs_info.maximum = max;
   abs_info.resolution = resolution;
   abs_info.fuzz = fuzz;
   abs_info.flat = flat;

   struct uinput_abs_setup abs_setup;
   abs_setup.code = code;
   abs_setup.absinfo = abs_info;

   if(ioctl(fd, UI_ABS_SETUP, &abs_setup) < 0) { // Set abs data
      perror("Failed to absolute info to uinput-device (old kernel?)");
      exit(1);
   }
}

int main(int argc, char** argv)
{
   // IP of reMarkable changeable as parameter:
   char hostname[30];
   strcpy(hostname, "10.11.99.1"); // Default IP

   if(argc > 1) {
      if(strlen(argv[1]) > 29) {
         fprintf(stderr, "The IP/Hostname is too long!");
         return 1;
      }

      strcpy(hostname, argv[1]);
   }


   // Create virtual tablet:
   int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

   ioctl(fd, UI_SET_EVBIT, EV_KEY);
   ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_PEN); // BTN_TOOL_PEN == 1 means that the pen is hovering over the tablet
   ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH); // BTN_TOUCH == 1 means that the pen is touching the tablet
   ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS);  // To satisfy libinput. Is not used.

   // See https://python-evdev.readthedocs.io/en/latest/apidoc.html#evdev.device.AbsInfo.resolution
   // Resolution = max(20967, 15725) / (21*10)  # Height of display is 21cm. Format is units/mm. => ca. 100 (99.84285714285714)
   // Tilt resolution = 12600 / ((math.pi / 180) * 140 (max angle)) (Format: units/radian) => ca. 5074 (5074.769042587292)
   ioctl(fd, UI_SET_EVBIT, EV_ABS);
   addAbsCapability(fd, ABS_PRESSURE, /*Value:*/ 0,     /*Min:*/ 0,     /*Max:*/ 4095,  /*Resolution:*/ 0,   /*Fuzz:*/ 0, /*Flat:*/ 0);
   addAbsCapability(fd, ABS_DISTANCE, /*Value:*/ 95,    /*Min:*/ 0,     /*Max:*/ 255,   /*Resolution:*/ 0,   /*Fuzz:*/ 0, /*Flat:*/ 0);
   addAbsCapability(fd, ABS_TILT_X,   /*Value:*/ 0,     /*Min:*/ -9000, /*Max:*/ 9000, /*Resolution:*/ 5074, /*Fuzz:*/ 0, /*Flat:*/ 0);
   addAbsCapability(fd, ABS_TILT_Y,   /*Value:*/ 0,     /*Min:*/ -9000, /*Max:*/ 9000, /*Resolution:*/ 5074, /*Fuzz:*/ 0, /*Flat:*/ 0);
   addAbsCapability(fd, ABS_X,        /*Value:*/ 11344, /*Min:*/ 0,     /*Max:*/ 20967, /*Resolution:*/ 100, /*Fuzz:*/ 0, /*Flat:*/ 0);
   addAbsCapability(fd, ABS_Y,        /*Value:*/ 10471, /*Min:*/ 0,     /*Max:*/ 15725, /*Resolution:*/ 100, /*Fuzz:*/ 0, /*Flat:*/ 0);

   struct uinput_setup usetup;
   memset(&usetup, 0, sizeof(usetup));
   usetup.id.bustype = BUS_VIRTUAL;
   //usetup.id.vendor = 0x1235; /* sample vendor */
   //usetup.id.product = 0x7890; /* sample product */
   usetup.id.version = 0x3;
   strcpy(usetup.name, "reMarkableTablet-FakePen");  // Has to end with "pen" to work in Krita!!!
   if(ioctl(fd, UI_DEV_SETUP, &usetup) < 0) {
      perror("Failed to setup uinput-device (old kernel?)");
      return 1;
   }
   
   if(ioctl(fd, UI_DEV_CREATE) < 0) {
      perror("Failed to create uinput-device");
      return 1;
   }

   void closeDevice() {
      ioctl(fd, UI_DEV_DESTROY);
      close(fd);
   }


   // Connect to reMarkable:
   int clientFd = socket(AF_INET, SOCK_STREAM, 0);   
   
   struct hostent *host = gethostbyname(hostname);

   struct sockaddr_in address;
   memset(&address, 0, sizeof(address));

   address.sin_family = AF_INET;
   bcopy((char *)host->h_addr, (char *)&address.sin_addr.s_addr, host->h_length);
   address.sin_port = htons(33333);
   
   if(connect(clientFd, (struct sockaddr*) &address, sizeof(address)) < 0) {
      perror("Failed to connect to the reMarkable");
      closeDevice();
      return 1;
   }

   printf("Connected\n");

   struct input_packet packet;
   size_t offset = 0;

   while(1) {
      ssize_t ret = read(clientFd, ((void*) &packet) + offset, sizeof(packet) - offset);
      if(ret < 0) {
         perror("Connection failed");
         closeDevice();
         return 1;
      }
      offset += ret;
      if(offset < sizeof(packet))
        continue;
      offset = 0;

      emit(fd, packet.type, packet.code, packet.value);
   }


   // Close virtual tablet:
   closeDevice();

   return 0;
}
