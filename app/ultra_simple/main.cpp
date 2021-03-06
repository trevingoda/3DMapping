/*
* Copyright (C) 2014  RoboPeak
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/*
*  RoboPeak Lidar System
*  Simple Data Grabber Demo App
*
*  Copyright 2009 - 2014 RoboPeak Team
*  http://www.robopeak.com
*
*  An ultra simple app to fetech RPLIDAR data continuously....
*
*/




#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>

#include "rplidar.h" //RPLIDAR standard sdk, all-in-one header
#include "utils.h"

#define FLOAT
#ifndef FLOAT
#define INT
#endif

#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

using namespace rp::standalone::rplidar;

bool checkRPLIDARHealth(RPlidarDriver * drv)
{
	u_result     op_result;
	rplidar_response_device_health_t healthinfo;


	op_result = drv->getHealth(healthinfo);
	if (IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
		printf("RPLidar health status : %d\n", healthinfo.status);
		if (healthinfo.status == RPLIDAR_STATUS_ERROR) {
			fprintf(stderr, "Error, rplidar internal error detected. Please reboot the device to retry.\n");
			// enable the following code if you want rplidar to be reboot by software
			// drv->reset();
			return false;
		}
		else {
			return true;
		}

	}
	else {
		fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
		return false;
	}
}

int main(int argc, const char * argv[]) {
	const char * opt_com_path = NULL;
	_u32         opt_com_baudrate = 115200;
	u_result     op_result;

	int file = 1, flag = 1, num = 1;
	float Ox, Oy;
	float angle, distance;
	float x, y, r;
#ifndef FLOAT
	int theta;
#else
	float theta;
#endif
	std::string fname;

	// read serial port from the command line...
	if (argc>1) opt_com_path = argv[1]; // or set to a fixed value: e.g. "com3" 

										// read baud rate from the command line if specified...
	if (argc>2) opt_com_baudrate = strtoul(argv[2], NULL, 10);


	if (!opt_com_path) {
#ifdef _WIN32
		// use default com port
		opt_com_path = "\\\\.\\com3";
#else
		opt_com_path = "/dev/ttyUSB0";
#endif
	}

	// create the driver instance
	RPlidarDriver * drv = RPlidarDriver::CreateDriver(RPlidarDriver::DRIVER_TYPE_SERIALPORT);

	if (!drv) {
		fprintf(stderr, "insufficent memory, exit\n");
		exit(-2);
	}


	// make connection...
	if (IS_FAIL(drv->connect(opt_com_path, opt_com_baudrate))) {
		fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
			, opt_com_path);
		goto on_finished;
	}



	// check health...
	if (!checkRPLIDARHealth(drv)) {
		goto on_finished;
	}


	// start scan...
	drv->startScan();

	// fetech result and print it out...
	while (1) {
		if (flag != 0) {

			/* need to modify the following to receive telemetry data from the pixhawk
			 * to calculate the distance moved by the scanner
			 */
			std::cout << "Enter scan origin offset x y (mm)" << std::endl;
			std::cin >> Ox >> Oy;

				while (num > 0) { //number of scans
					std::ofstream myfile;
					fname = "scan";

					myfile.open(fname, std::fstream::out | std::fstream::app);		//open file with name "scan_#"
																					//file++;

					rplidar_response_measurement_node_t nodes[360 * 2];
					size_t   count = _countof(nodes);

					op_result = drv->grabScanData(nodes, count);

					//myfile << Ox << '\0' << Oy << '\0' << std::endl;

					if (IS_OK(op_result)) {
						drv->ascendScanData(nodes, count);
						for (int pos = 0; pos < (int)count; ++pos) {
							r = (nodes[pos].distance_q2 / 4.0f);
#ifdef FLOAT

							//polar to cartesian

							theta = ((nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / 64.0f);
							std::cout << theta << ' ' << r << std::endl;
							x = r * cos(theta*0.0174533);
							y = r * sin(theta*0.0174533);
							std::cout << '\t' << x << ' ' << y << std::endl;
#else
							theta = (int)((nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / 64.0f);
#endif
							//add the offset to the cartesian
							x = x + Ox;
							y = y + Oy;

							//cartesian back to polar
							angle = atan2(y, x);
							angle = angle * 57.2958;
							distance = sqrt(x*x + y*y);

							//print to file
							myfile << angle << ' ';
							myfile << distance << std::endl;

						}
				}
				myfile.close();
				num--;
			}
			num = 1;
			std::cout << "take next scan? (0) to quit ";		//check if there are more scans to take
			std::cin >> flag;
			}
			else break;
	}
    // done!
on_finished:
	RPlidarDriver::DisposeDriver(drv);
	return 0;
}
