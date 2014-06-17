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
#include <sys/time.h>
#define RESPONSE_TAG 100
#define	REQUEST_MASTER_TAG 101
#define	REQUEST_ROOM_TAG 102
#define RELEASE_ROOM_TAG 103
#define RELEASE_MASTER_TAG 104

using namespace std;

typedef pair<int, int> P;	// <timestamp><tid>

int main(int argc, char **argv)
{
	int mastersNum, roomsNum, projectorsNum;

	cout << "Podaj kolejno: liczbę mistrzów, liczbę sal, liczbę projektorów. Następnie podaj ':' i wypisz moce kolejnych mistrzów, bądź 'x' i podaj jedną moc dla wszystkich" << endl;
	cin >> mastersNum >> roomsNum >> projectorsNum;
	roomsNum = min(roomsNum,projectorsNum);

	int tmp;
	char znak;
	cin >> znak;

	vector<int> mastersPowerInit;

	if(znak=='x'){
		cin >> tmp;
		for(int i=0;i<mastersNum;i++)
			mastersPowerInit.push_back(tmp);
	}
	else if(znak==':'){
		for(int i=0;i<mastersNum;i++){
			cin >> tmp;
			mastersPowerInit.push_back(tmp);
		}
	}

	vector< set<P, greater<P> > > mastersQ(mastersNum, set<P, greater<P> >());
	vector< set<P, greater<P> > > roomsQ(roomsNum, set<P, greater<P> >());

	set<int> responses;

	vector< P > mastersPower(mastersNum, P(20, 0) );//P<power,version>
	for(int i=0;i<mastersNum;i++)
		mastersPower[i].first=mastersPowerInit[i];
	int lecturesDone = 0;
	
	
	int tid,size;
	MPI_Status status;
	MPI_Request request;
	int flag=-1;

	MPI_Init(&argc, &argv); //Musi być w każdym programie na początku
	MPI_Comm_size( MPI_COMM_WORLD, &size );
	MPI_Comm_rank( MPI_COMM_WORLD, &tid );

	cout<<"Id: "<<tid<<" from "<<size<<endl;
	
	
	enum {vacant, gettingMaster, gettingRoom, lecture, meditate} stan;
	stan = vacant;
	int myMaster=-1, myRoom=-1;
	long lectureEnd;
	long meditateEnd;
    
	int msg4[4] = {0};	// <zasób><timestamp><moc><version>
	int msg[4] = {0};	// <zasób><timestamp><moc><version>
		
	int min_size, min_index;
	
	struct timeval tv;

	srand(time(NULL)+tid);
	while(1){
		sleep(1);	
		
		switch(stan){
			case vacant:				
				min_size=100000;
				min_index=0;
				for(int i=0;i<mastersNum;i++)
					if(mastersQ[i].size()<min_size) {
						min_size = mastersQ[i].size();
						min_index = i;
					}
				myMaster=min_index;				
				
				mastersQ[myMaster].insert(P(lecturesDone, tid));
				gettimeofday(&tv, NULL);
				cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": waiting for MyMaster: "<<myMaster<<endl;
				msg[0]=myMaster;
				msg[1]=lecturesDone;
				for(int i=0; i<size; i++)
					if(i!=tid) {
						MPI_Send(&msg, 4, MPI_INT, i, REQUEST_MASTER_TAG, MPI_COMM_WORLD );
						gettimeofday(&tv, NULL);
						cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": send REQUEST_MASTER_TAG to: "<<i<<endl;
					}
				stan=gettingMaster;
				break;
			case gettingMaster:
				if(responses.size() == size-1) {
					responses.clear();
					
					min_size=100000;
					min_index=0;
					for(int i=0;i<roomsNum;i++)
						if(roomsQ[i].size()<min_size) {
							min_size = roomsQ[i].size();
							min_index = i;
						}
					myRoom=min_index;				

					roomsQ[myRoom].insert(P(lecturesDone, tid));
					gettimeofday(&tv, NULL);
					cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": waiting for MyRoom: "<<myRoom<<endl;
					msg[0]=myRoom;
					msg[1]=lecturesDone;
					for(int i=0; i<size; i++)
						if(i!=tid) {
							MPI_Send(&msg, 4, MPI_INT, i, REQUEST_ROOM_TAG, MPI_COMM_WORLD );
							gettimeofday(&tv, NULL);
							cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": send REQUEST_ROOM_TAG to: "<<i<<endl;
						}

					stan=gettingRoom;
				}
				break;
			case gettingRoom:
				if(responses.size() == size-1) {
					responses.clear();
					lectureEnd=time(NULL) + rand()%3;
					stan=lecture;
					gettimeofday(&tv, NULL);
					cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": lecture START master: " << myMaster << "  room:"<<myRoom<<endl;
				}
				break;
			case lecture:
				if(time(NULL)>=lectureEnd) {
					gettimeofday(&tv, NULL);
					cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": lecture STOP master: " << myMaster << "  room:"<<myRoom<<endl;
					roomsQ[myRoom].erase(P(myRoom,lecturesDone));
					msg[0]=myRoom;
					msg[1]=lecturesDone;
					myRoom=-1;
					for(int i=0; i<size; i++)
						if(i!=tid) {
							MPI_Send(&msg, 4, MPI_INT, i, RELEASE_ROOM_TAG, MPI_COMM_WORLD );
							gettimeofday(&tv, NULL);
							cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": send RELESAE_ROOM_TAG  to:"<<i<<endl;
						}
					lecturesDone++;
					mastersPower[myMaster].first--;
					if(mastersPower[myMaster].first > 0) {
						mastersPower[myMaster].first--;
						mastersPower[myMaster].second++;
						msg[0]=myMaster;
						msg[1]=lecturesDone;
						msg[2]=mastersPower[myMaster].first;
						msg[3]=mastersPower[myMaster].second;
						myMaster=-1;
						for(int i=0; i<size; i++)
							if(i!=tid){
								MPI_Send(&msg, 4, MPI_INT, i, RELEASE_MASTER_TAG, MPI_COMM_WORLD );
								gettimeofday(&tv, NULL);
								cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": send RELEASE_MASTER_TAG to: " << i << "  (power " <<msg[2]<<" v"<< msg[3]<< ")"<<endl;
							}
						stan=vacant;
					}
					else {
						meditateEnd=time(NULL)+rand()%3;
						stan=meditate;
					}
				}
				break;
			case meditate:
				// TODO // Hmm.. chyba gotowe , nie? Po co to TODO?
				if(time(NULL)>=meditateEnd) {
						mastersPower[myMaster].first=50;
						mastersPower[myMaster].second++;
						msg[0]=myMaster;
						msg[1]=lecturesDone;
						msg[2]=mastersPower[myMaster].first;
						msg[3]=mastersPower[myMaster].second;
						myMaster=-1;
						for(int i=0; i<size; i++)
							if(i!=tid) {
								MPI_Send(&msg, 4, MPI_INT, i, RELEASE_MASTER_TAG, MPI_COMM_WORLD );
								gettimeofday(&tv, NULL);
								cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": send RELEASE_MASTER_TAG to: " << i << "  (power " <<msg[2]<<" v"<< msg[3]<< ")"<<endl;
							}
						stan=vacant;
				}
				break;
		}
		
		
		
		// OBSLUGA REQUESTA
		//cout<<tid<<": before"<<endl;
		if(flag!=0) {
			//cout<<tid<<": in"<<endl;
			MPI_Irecv(msg4, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
			flag=0;
		}
		//cout<<tid<<": test"<<endl;
		//printf("Otrzymalem %d %d od %d, WARTOSC = %d\n", msg[0], msg[1], status.MPI_SOURCE, msg[2]);
		MPI_Test(&request, &flag, &status);
		//cout<<tid<<": after"<<endl;
		
		/*	
		MPI_Recv(msg4, 4, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		flag=1;	
		*/
		if(flag!=0)
			if(status.MPI_SOURCE!=-1){				
			//cout<<tid<<": "<<status.MPI_SOURCE<<endl;
			switch(status.MPI_TAG) {
				case REQUEST_ROOM_TAG:
					gettimeofday(&tv, NULL);
					cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": REQUEST from "<<status.MPI_SOURCE<<" about "<<msg4[0]<<" "<<msg4[1]<<endl;
					if(msg4[0]==myRoom && stan==gettingRoom){
						//jeżeli tamten lepszy, to wyślij sygnał wolny
						//jeżeli tamten gorszy, to ingnoruj
						if(msg4[1]<lecturesDone || (msg4[1] == lecturesDone && status.MPI_SOURCE < tid) ) {
							MPI_Send(&msg4, 4, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
							gettimeofday(&tv, NULL);
							cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": Send response to "<<status.MPI_SOURCE<<endl;
						}
					}
					else if(msg4[0]!=myRoom){
						// odeślij sygnał wolny
						MPI_Send(&msg4, 4, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
						gettimeofday(&tv, NULL);
						cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": Send response to "<<status.MPI_SOURCE<<endl;
					}
					
					// dodaj do kolejki
					roomsQ[msg4[0]].insert(P(msg4[1], status.MPI_SOURCE));
				break;
				case REQUEST_MASTER_TAG:
					gettimeofday(&tv, NULL);
					cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": REQUEST from "<<status.MPI_SOURCE<<" about "<<msg4[0]<<" "<<msg4[1]<<endl;
					if(msg4[0]==myMaster && stan==gettingMaster){
						if(msg4[1]<lecturesDone || (msg4[1] == lecturesDone && status.MPI_SOURCE < tid) ) {
							MPI_Send(&msg4, 4, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
							gettimeofday(&tv, NULL);
							cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": Send response to "<<status.MPI_SOURCE<<endl;
						}
					}
					else if(msg4[0]!=myMaster){
						MPI_Send(&msg4, 4, MPI_INT, status.MPI_SOURCE, RESPONSE_TAG, MPI_COMM_WORLD );
						gettimeofday(&tv, NULL);
						cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": Send response to "<<status.MPI_SOURCE<<endl;
					}
					
					mastersQ[msg4[0]].insert(P(msg4[1], status.MPI_SOURCE));
				break;
				case RESPONSE_TAG:
					responses.insert(status.MPI_SOURCE);
					gettimeofday(&tv, NULL);
					cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": RESPONSE"<<endl;
				break;
				case RELEASE_ROOM_TAG:
					gettimeofday(&tv, NULL);
					cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": RELEASE_ROOM_TAG from: " << status.MPI_SOURCE <<endl;
					if(stan==gettingRoom && msg4[0]==myRoom)
						responses.insert(status.MPI_SOURCE);
					// usuwanie z kolejki:
					roomsQ[msg4[0]].erase(P(msg4[1],status.MPI_SOURCE));
				break;
				case RELEASE_MASTER_TAG:
					gettimeofday(&tv, NULL);
					cout<<tv.tv_sec*1000000+tv.tv_usec<<" # "<<tid<<": RELEASE_MASTER_TAG from: " << status.MPI_SOURCE << "  (power " <<msg4[2]<<" v"<< msg4[3]<< ")"<<endl;
					if(stan==gettingMaster && msg4[0]==myMaster)
						responses.insert(status.MPI_SOURCE);
					// usuwanie z kolejki:
					mastersQ[msg4[0]].erase(P(msg4[1],status.MPI_SOURCE));
					if(mastersPower[msg4[0]].second<msg4[3]) {
						mastersPower[msg4[0]].first=msg4[2];
						mastersPower[msg4[0]].second=msg4[3];
					}
				break;
				default:
					cout<<tid<<": ERROR"<<status.MPI_TAG<<endl;

			}
			flag=-1;

		}
		

	}


	MPI_Finalize(); // Musi być w każdym programie na końcu
}


/*
int getMaster(){
	int min_size=100000, min_index=0;
	for(int i=0;i<mastersNum;i++)
		if(mastersQ[i].size()<min_size) {
			min_size = mastersQ[i].size();
			min_index = i;
		}
	
	return min_index;
}


int getRoom(){
	int min_size=100000, min_index=0;
	for(int i=0;i<roomsNum;i++)
		if(roomsQ[i].size()<min_size) {
			min_size = roomsQ[i].size();
			min_index = i;
		}
	
	return min_index;
}
*/
