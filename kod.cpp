#include <mpi.h>
#include <stdio.h>
#include <ctime>
#include <vector>
#include <utility>
#include <set>
#include <queue>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#define RESPONSE_TAG 100
#define	REQUEST_MASTER_TAG 101
#define	REQUEST_ROOM_TAG 102
#define RELEASE_ROOM_TAG 103
#define RELEASE_MASTER_TAG 104

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
	stan = vacant;
	int myMaster=-1, myRoom=-1;
	long lectureEnd;
    
	/*
	int msg2[2];	// <zasób><timestamp>
	int msg1;
	int msg5[5];	// <0 Master/1 Room><zasób><timestamp><moc><version>
	*/
	int msg4[4];	// <zasób><timestamp><moc><version>
		
		
	srand(time(NULL)+tid);
	while(1){
		sleep(1);	
		
		switch(stan){
			case vacant:
				//myMaster = getMaster();	// dodaje do własnej kolejki i wysyła żądania do wszystkich innych
// TUTAJ->>>	//stan=gettingMaster;
				//myRoom = getRoom(); // dodaje do własnej kolejki i wysyła żądania do wszystkich innych
				myRoom = rand()%roomsNum;
				roomsQ[myRoom].insert(P(lecturesDone, tid));
				cout<<tid<<": MyRoom: "<<myRoom<<endl;
				msg4[0]=myRoom;
				msg4[1]=lecturesDone;
				for(int i=0; i<size; i++)
					if(i!=tid)
						MPI_Send(&msg4, 4, MPI_INT, i, REQUEST_ROOM_TAG, MPI_COMM_WORLD );

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
					msg4[0]=myRoom;
					msg4[1]=lecturesDone;
					myRoom=-1;
					myMaster=-1;
					for(int i=0; i<size; i++)
						if(i!=tid)
							MPI_Send(&msg4, 4, MPI_INT, i, RELEASE_ROOM_TAG, MPI_COMM_WORLD );
					lecturesDone++;
					stan=vacant;
				}
				break;
			case meditate:
				// TODO
				break;
		}
		
		
		// OBSLUGA REQUESTA
		if(flag!=0) {
			MPI_Irecv(msg4, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
			flag=0;
		}
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		MPI_Test(&request, &flag, &status);
		
		if(flag){				
			switch(status.MPI_TAG) {
				case REQUEST_ROOM_TAG:
					cout<<tid<<": REQUEST from "<<status.MPI_SOURCE<<" about "<<msg4[0]<<" "<<msg4[1]<<endl;
					if(msg4[0]==myRoom && stan==gettingRoom){
						// odeślij znacznik czasowy
						//jeżeli tamten lepszy, to sygnał wolny -1
						//jeżeli tamten gorszy, to ingnore
						//ew. tablice
						//msg1 = lecturesDone;
						//MPI_Send(&msg1, 1, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
						msg4[0]=-1;
						if(msg4[1]<lecturesDone || (msg4[1] == lecturesDone && status.MPI_SOURCE < tid) ) {
							MPI_Send(&msg4, 4, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
							cout<<tid<<": Send response to "<<status.MPI_SOURCE<<endl;
						}
					}
					else if(msg4[0]!=myRoom){
						// odeślij sygnał wolny -1
						msg4[0] = -1;
						MPI_Send(&msg4, 4, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
						cout<<tid<<": Send response to "<<status.MPI_SOURCE<<endl;
					}
					
					// dodaj do kolejki
					roomsQ[msg4[0]].insert(P(msg4[1], status.MPI_SOURCE));
				break;
				case RESPONSE_TAG:
					responses.insert(status.MPI_SOURCE);
					cout<<tid<<": RESPONSE"<<endl;
				break;
				case RELEASE_ROOM_TAG:
					if(stan==gettingRoom && msg4[0]==myRoom)
						responses.insert(status.MPI_SOURCE);
					// usuwanie z kolejki:, może się przydać timestamp
					roomsQ[msg4[0]].erase(P(msg4[1],status.MPI_SOURCE));
				break;

			}
			flag=-1;

		}
			/*
		
		
		
		// ODBIOR RESPONSOW
		MPI_Irecv(&msg1, 1, MPI_INT, MPI_ANY_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD, &request);
		MPI_Test(&request, &flag, &status);
		
		if(flag){
			responses.insert(status.MPI_SOURCE);
			cout<<tid<<": RESPONSE"<<endl;
		}
		
		// ODBIOR RELEASOW
		MPI_Irecv(msg5, 5, MPI_INT, MPI_ANY_SOURCE, RELEASE_TAG, MPI_COMM_WORLD, &request);
		MPI_Test(&request, &flag, &status);
		
		if(flag){
			if(msg5[0]==1) { //if room
				if(stan==gettingRoom && msg5[1]==myRoom)
					responses.insert(status.MPI_SOURCE);
				// usuwanie z kolejki:, może się przydać timestamp
				roomsQ[msg5[0]].erase(P(msg5[2],status.MPI_SOURCE));
			}
		}
		*/
		

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
