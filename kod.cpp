#include <mpi.h>
#include <stdio.h>
#include <ctime>
#include <vector>
#include <utility>
#include <set>
#include <queue>
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
	vector< priority_queue<P, vector <P>, greater<P> > > mastersQ(mastersNum, priority_queue<P, vector <P>, greater<P> >());
	vector< priority_queue<P, vector <P>, greater<P> > > roomsQ(roomsNum, priority_queue<P, vector <P>, greater<P> >());

	set<int> responses;

	vector< pair<int,int> > mastersPower(mastersNum, pair<int,int>(0, 50) );	
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

	enum {vacant, gettingMaster, gettingRoom, lecture, meditate} stan;
	int myMaster=-1, myRoom=-1;
    long lectureEnd;
    
	while(1){
		
		switch(stan){
			case vacant:
				myMaster = getMaster();	// dodaje do własnej kolejki i wysyła żądania do wszystkich innych
// TUTAJ->>>	//stan=gettingMaster;
				stan = gettingRoom;
				break;
			case gettingMaster:
				if(responses.size() == size-1) {
					responses.clean();
					myRoom = getRoom(); // dodaje do własnej kolejki i wysyła żądania do wszystkich innych
					stan=gettingRoom;
				}
				break;
			case gettingRoom:
				if(responses.size() == size-1) {
					responses.clean();
					lectureEnd=time() + 3;
					stan=lecture;
				}
				break;
			case lecture:
				// TODO
				break;
			case meditate:
				// TODO
				break;
		}
		
		
		int msg2[2];	// <zasób><timestamp>
		int msg1;
		int msg4[4];	// <0 Master/1 Room><zasób><moc><version>
		
		
		// OBSLUGA REQUESTA
		MPI_iRecv(msg2, 2, MPI_INT, MPI_ANY_SOURCE, REQUEST_ROOM_TAG, MPI_COMM_WORLD, &request);
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		MPI_Test(&request, &flag, &status);
		
		if(flag){				
			if(msg2[0]==myRoom && stan==gettingRoom){
				// odeślij znacznik czasowy
				//jeżeli tamten lepszy, to sygnał wolny -1
				//jeżeli tamten gorszy, to ingnore
				//ew. tablice
				//msg1 = lecturesDone;
				//MPI_Send(&msg1, 1, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
				msg1=-1;
				if(msg2[1]<lecturesDone || (msg2[1] == lecturesDone && status.MPI_SOURCE < tid) )
					MPI_Send(&msg1, 1, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
			}
			else if(msg2[0]!=myRoom){
				// odeślij sygnał wolny -1
				msg1 = -1;
				MPI_Send(&msg1, 1, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
			}
			
			// dodaj do kolejki
			roomQ[msg2[0]].push(pair<int,int>(msg2[1], status.MPI_SOURCE));

//			MPI_Send( msg, 3, MPI_INT, (tid+1)%size, MSG_TAG, MPI_COMM_WORLD );
			//printf("Wyslalem %d %d, WARTOSC = %d\n", msg[0], msg[1], msg[2]);
		}
		
		
		
		// ODBIOR RESPONSOW
		MPI_iRecv(&msg1, 1, MPI_INT, MPI_ANY_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD, &request);
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		MPI_Test(&request, &flag, &status);
		
		if(flag){
			responses.insert(status.MPI_SOURCE);
		}
		
		// ODBIOR RELEASOW
		MPI_iRecv(msg4, 1, MPI_INT, MPI_ANY_SOURCE, RELEASE_TAG, MPI_COMM_WORLD, &request);
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		MPI_Test(&request, &flag, &status);
		
		if(flag){
			if(mag4[0]==1) {
				if(stan==gettingRoom)
					responses.insert(status.MPI_SOURCE);
				// usuwanie z kolejki:, może się przydać timestamp
			}
		}
		

	}


	MPI_Finalize(); // Musi być w każdym programie na końcu
}



int getMaster(){
	
	return -1;
}


int getRoom(){
	return -1;
}
