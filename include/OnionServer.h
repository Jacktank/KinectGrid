#ifndef ONIONSERVER_H
#define ONIONSERVER_H

#include <pthread.h>
#include <png.h>

#include <onion/log.h>
#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/extras/png.h>

/* Declare jSON data object here
 *
 *
 */


//thread function
static void* start_myonion_server(void* arg){
	printf("Onion server: Start listening.\n");
	onion_listen((onion*)arg);//loop
	printf("Onion server: Stop listening.\n");
}

class OnionServer{
	private:
	public:	
		onion* m_ponion;
		pthread_t m_pthread;
	public:
		OnionServer():
			m_ponion( onion_new(O_THREADED) ),
			m_pthread()
		{
			//start_server();
		}
		~OnionServer()
		{
			if(m_ponion != NULL) stop_server();
		}

		int start_server();

		int stop_server();
};

#endif
