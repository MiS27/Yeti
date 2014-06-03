#include <mpi.h>
#include <stdio.h>
#include <ctime>
#include <vector>
#include <utility>
#include <set>
#include <queue>
#include <cstdlib>
#include <iostream>
#define RESPONSE_TAG 100
#define	REQUEST_MASTER_TAG 101
#define	REQUEST_ROOM_TAG 102
#define RELEASE_TAG 103

using namespace std;

typedef pair<int, int> P;	// <timestamp><tid>


int getMaster();
int getRoom();

int main(int argc, char **argv)
{
	int mastersNum, roomsNum;
	mastersNum = 10;
	roomsNum = 10;
	vector< set<P, greater<P> > > mastersQ(mastersNum, set<P, greater<P> >());
	vector< set<P, greater<P> > > roomsQ(roomsNum, set<P, greater<P> >());

	set<int> responses;

	vector< P > mastersPower(mastersNum, P(0, 50) );	
	int lecturesDone = 0;
	
	
	int tid,size;
	MPI_Status status;
	MPI_Request request;
	int flag;

	MPI_Init(&argc, &argv); //Musi być w każdym programie na początku
	//printf("Checking!\n");
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &tid );
	//printf("My id is %d from %d\n",tid, size);

	cout<<"Id: "<<tid<<" from "<<size<<endl;
	enum {vacant, gettingMaster, gettingRoom, lecture, meditate} stan;
	int myMaster=-1, myRoom=-1;
	long lectureEnd;
    
	int msg2[2];	// <zasób><timestamp>
	int msg1;
	int msg5[5];	// <0 Master/1 Room><zasób><timestamp><moc><version>
		
		
	srand(time(NULL)+tid);
	while(1){
	
		
		switch(stan){
			case vacant:
				//myMaster = getMaster();	// dodaje do własnej kolejki i wysyła żądania do wszystkich innych
// TUTAJ->>>	//stan=gettingMaster;
				//myRoom = getRoom(); // dodaje do własnej kolejki i wysyła żądania do wszystkich innych
				myRoom = rand()%roomsNum;
				roomsQ[myRoom].insert(P(lecturesDone, tid));
				cout<<tid<<": MyRoom: "<<myRoom<<endl;
				msg2[0]=myRoom;
				msg2[1]=lecturesDone;
				for(int i=0; i<size; i++)
					if(i!=tid)
						MPI_Send(&msg2, 2, MPI_INT, i, REQUEST_ROOM_TAG, MPI_COMM_WORLD );

				stan=gettingRoom;
				break;
			case gettingMaster:
				if(responses.size() == size-1) {
					responses.clear();
					//myRoom = getRoom(); // dodaje do własnej kolejki i wysyła żądania do wszystkich innych
					stan=gettingRoom;
				}
				break;
			case gettingRoom:
				if(responses.size() == size-1) {
					responses.clear();
					lectureEnd=time(NULL) + 3;
					stan=lecture;
					cout<<tid<<": lecture room:"<<myRoom<<endl;
				}
			/*	
				else
					cout<<"responses: "<<responses.size()<<endl;
			*/	
				break;
			case lecture:
				// TODO
				if(time(NULL)>=lectureEnd) {
					cout<<tid<<": lecture done room:"<<myRoom<<endl;
					roomsQ[myRoom].erase(P(myRoom,lecturesDone));
					msg5[0]=1;
					msg5[1]=myRoom;
					msg5[2]=lecturesDone;
					for(int i=0; i<size; i++)
						if(i!=tid)
							MPI_Send(&msg5, 5, MPI_INT, i, RELEASE_TAG, MPI_COMM_WORLD );
					lecturesDone++;
					stan=vacant;
				}
				break;
			case meditate:
				// TODO
				break;
		}
		
		
		// OBSLUGA REQUESTA
		MPI_Irecv(msg2, 2, MPI_INT, MPI_ANY_SOURCE, REQUEST_ROOM_TAG, MPI_COMM_WORLD, &request);
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		MPI_Test(&request, &flag, &status);
		
		if(flag){				
			cout<<tid<<": REQUEST from "<<status.MPI_SOURCE<<" about "<<msg2[0]<<" "<<msg2[1]<<endl;
			if(msg2[0]==myRoom && stan==gettingRoom){
				// odeślij znacznik czasowy
				//jeżeli tamten lepszy, to sygnał wolny -1
				//jeżeli tamten gorszy, to ingnore
				//ew. tablice
				//msg1 = lecturesDone;
				//MPI_Send(&msg1, 1, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
				msg1=-1;
				if(msg2[1]<lecturesDone || (msg2[1] == lecturesDone && status.MPI_SOURCE < tid) ) {
					MPI_Send(&msg1, 1, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
					cout<<tid<<": Send response to "<<status.MPI_SOURCE<<endl;
				}
			}
			else if(msg2[0]!=myRoom){
				// odeślij sygnał wolny -1
				msg1 = -1;
				MPI_Send(&msg1, 1, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
				cout<<tid<<": Send response to "<<status.MPI_SOURCE<<endl;
			}
			
			// dodaj do kolejki
			roomsQ[msg2[0]].insert(P(msg2[1], status.MPI_SOURCE));

//			MPI_Send( msg, 3, MPI_INT, (tid+1)%size, MSG_TAG, MPI_COMM_WORLD );
			//printf("Wyslalem %d %d, WARTOSC = %d\n", msg[0], msg[1], msg[2]);
		}
		
		
		
		// ODBIOR RESPONSOW
		MPI_Irecv(&msg1, 1, MPI_INT, MPI_ANY_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD, &request);
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		MPI_Test(&request, &flag, &status);
		
		if(flag){
			responses.insert(status.MPI_SOURCE);
			cout<<tid<<": RESPONSE"<<endl;
		}
		
		// ODBIOR RELEASOW
		MPI_Irecv(msg5, 5, MPI_INT, MPI_ANY_SOURCE, RELEASE_TAG, MPI_COMM_WORLD, &request);
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		MPI_Test(&request, &flag, &status);
		
		if(flag){
			if(msg5[0]==1) { //if room
				if(stan==gettingRoom)
					responses.insert(status.MPI_SOURCE);
				// usuwanie z kolejki:, może się przydać timestamp
				roomsQ[msg5[0]].erase(P(msg5[2],status.MPI_SOURCE));
			}
		}
		

	}


	MPI_Finalize(); // Musi być w każdym programie na końcu
}



int getMaster(){
	
	return -1;
}


int getRoom(){
	/*
	int myRoom = rand()%roomsNum;
	roomsQ[myRoom].insert(P(lecturesDone, tid));
	return myRoom;
	*/
	return -1;
}
