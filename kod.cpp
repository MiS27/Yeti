#include <mpi.h>
#include <stdio.h>
#include <vector>
#include <utility>
#define ROOT 0
#define MSG_TAG 100


int main(int argc, char **argv)
{
	int mastersNum, roomsNum;
	mastersNum = 10;
	roomsNum = 10;
	vector< vector<int> > mastersQ(mastersNum, vector<int>()), roomsQ(roomsNum, vector<int>());
	vector< pair<int,int> > mastersPower(mastersNum, pair<int,int>(0, 50) );	
	int lecturesDone = 0;
	
	
	int tid,size;
	MPI_Status status;

	MPI_Init(&argc, &argv); //Musi być w każdym programie na początku
	//printf("Checking!\n");
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &tid );
	//printf("My id is %d from %d\n",tid, size);

	int msg[3];


	while(1){
		
		

		msg[0] = tid;
		msg[1] = size;
		msg[2] = 0;

		MPI_Send( msg, 3, MPI_INT, tid+1, MSG_TAG, MPI_COMM_WORLD );
		printf("\t\t\tWyslalem %d %d, WARTOSC = %d\n", msg[0], msg[1], msg[2]);

		MPI_Recv(msg, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("\t\t\t\Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);





		MPI_Recv(msg, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		msg[0] = tid;
		msg[1] = size;
		msg[2] = msg[2]+1;

		MPI_Send( msg, 3, MPI_INT, (tid+1)%size, MSG_TAG, MPI_COMM_WORLD );
		//printf("Wyslalem %d %d, WARTOSC = %d\n", msg[0], msg[1], msg[2]);

	}


	MPI_Finalize(); // Musi być w każdym programie na końcu
}
