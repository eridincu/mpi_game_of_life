/* Mehmet Erdinç Oğuz
 * 2017400267
 * Compiling / Working / Periodic / Checkered
*/
#include <iostream>
#include <vector>
#include <mpi.h>
#include <math.h>
#include <fstream>
#include <cstdlib>

using namespace std;

const int GRID_SIDE = 360;
int main(int argc,char* argv[]) {

    if(argc != 4){
        cout << "Input file,output file and turn don't given properly."<<endl;
    }

    int p_rank,p_size;
    // Initialize MPI
    MPI_Status status;
    MPI_Init(&argc,&argv);

    MPI_Comm_rank(MPI_COMM_WORLD,&p_rank);
    MPI_Comm_size(MPI_COMM_WORLD,&p_size);
    // Initialize some values that I will use in the future
    int data_size = GRID_SIDE * GRID_SIDE / (p_size - 1); // data size for each slave process
    int side_length = sqrt(data_size); // side length of each 2d array used in slave processes
    int num_matrix_at_side = sqrt(p_size - 1); // number of processes used at a side of input 2D array

    //Master process enter this block other threads use else block.
    if(p_rank == 0){
        int init_data[GRID_SIDE][GRID_SIDE];
        int out_data[GRID_SIDE][GRID_SIDE];
        ifstream input(argv[1]);
        ofstream output(argv[2]);

        // Take input.
        for (int i = 0; i < GRID_SIDE; ++i) {
            for (int j = 0; j < GRID_SIDE; ++j) {
                int data;
                input >> data;
                init_data[i][j] = data;
            }
        }

        // Send data to slaves.
        vector<int> linear_send_slave_data(data_size);
        int row = 0;
        int column = 0;
        int rank = 1;
        int count = 0;
        while (row < num_matrix_at_side ){
            if(column == num_matrix_at_side){
                row++;
                column=0;
            } else {
                while(column < num_matrix_at_side){
                    for (int i= side_length * (row); i < side_length * (row + 1); ++i) {
                        for (int j= side_length * (column); j < side_length * (column + 1); j++) {
                            linear_send_slave_data[count] = init_data[i][j];
                            ++count;
                        }
                    }
                    column++;
                    count = 0;
                    MPI_Send(&linear_send_slave_data[0], data_size, MPI_INT, rank, rank, MPI_COMM_WORLD);
                    rank++;
                }
            }
        }
        // Master gets the final version of the map from each slave.
        vector<int> linear_final_data(data_size);
        for (int k = 1; k < p_size; ++k) {
            MPI_Recv(&linear_final_data[0], data_size, MPI_INT, k, 0, MPI_COMM_WORLD, &status);
            int rowOfReceived = (k-1) / num_matrix_at_side;
            int columnOfReceived = (k-1) % num_matrix_at_side;
            int count = 0;
            for(int l = side_length * rowOfReceived; l < side_length * (rowOfReceived + 1); ++l){
                for(int m = side_length * columnOfReceived; m < side_length * (columnOfReceived + 1); ++m) {
                    out_data[l][m] = linear_final_data[count];
                    ++count;
                }
            }
        }
        // Print the aggregated final result of the map to the file.
        for(int n = 0; n < GRID_SIDE; ++n){
            for(int k = 0; k < GRID_SIDE; ++k){
                output << out_data[n][k] << " ";
            }
            output << "\n";
        }

    } else {
        vector<int> linear_received_data(data_size);
        //Initial state of the map received as a linear vector.
        MPI_Recv(&linear_received_data[0], data_size, MPI_INT, 0, p_rank, MPI_COMM_WORLD, &status);
        int *current_data[side_length];
        for(int i = 0; i < side_length; ++i)
            current_data[i] = new int[side_length];

        int count = 0;
        for(int i = 0; i < side_length; ++i) {
            for (int j = 0; j < side_length; ++j) {
                current_data[i][j] = linear_received_data[count];
                count++;
            }
        }
        // MAIN WHILE LOOP TO SIMULATE THE STATES
        int t = atoi(argv[3]);
        while(t > 0) {
            t--;

            // The four corners of the matrix is prepared for sending.
            int send_top_left_corner, send_top_right_corner, send_bottom_left_corner,send_bottom_right_corner;
            send_top_left_corner = current_data[0][0];
            send_top_right_corner = current_data[0][side_length - 1];
            send_bottom_left_corner = current_data[side_length - 1][0];
            send_bottom_right_corner = current_data[side_length - 1][side_length - 1];

            // The four sides of the matrix is prepared for sending.
            vector<int> send_right_data;
            vector<int> send_left_data;
            vector<int> send_top_data;
            vector<int> send_bottom_data;
            for (int i = 0; i < side_length; ++i) {
                send_top_data.push_back(current_data[0][i]);
                send_bottom_data.push_back(current_data[side_length - 1][i]);
                send_left_data.push_back(current_data[i][0]);
                send_right_data.push_back(current_data[i][side_length - 1]);
            }
            // Values below are used to store the data received.
            int recv_top_left_corner,recv_top_right_corner,recv_bottom_left_corner,recv_bottom_right_corner;
            vector<int> receive_left_data;
            vector<int> receive_right_data;
            vector<int> receive_bottom_data;
            vector<int> receive_top_data;
            receive_top_data.resize(side_length);
            receive_left_data.resize(side_length);
            receive_bottom_data.resize(side_length);
            receive_right_data.resize(side_length);

            // With this if else block all threads make their left-right information exchange.
            if(p_rank % 2 == 1) {
                int left_side = p_rank % num_matrix_at_side == 1 ? p_rank + num_matrix_at_side - 1 : p_rank - 1;
                int right_side = p_rank + 1;
                //Send right data.
                MPI_Send(&send_right_data[0], side_length, MPI_INT, right_side, 6, MPI_COMM_WORLD);
                //Send left data.
                MPI_Send(&send_left_data[0], side_length, MPI_INT, left_side, 4, MPI_COMM_WORLD);
                //Receive right data.
                MPI_Recv(&receive_right_data[0], side_length, MPI_INT, right_side, 4, MPI_COMM_WORLD, &status);
                //Receive left.
                MPI_Recv(&receive_left_data[0], side_length, MPI_INT, left_side, 6, MPI_COMM_WORLD, &status);
            } else {
                int left_side = p_rank - 1;
                int right_side= p_rank % num_matrix_at_side == 0 ? p_rank - num_matrix_at_side + 1 : p_rank + 1;
                //Receive left.
                MPI_Recv(&receive_left_data[0], side_length, MPI_INT, left_side, 6, MPI_COMM_WORLD, &status);
                //Receive right.
                MPI_Recv(&receive_right_data[0], side_length, MPI_INT, right_side, 4, MPI_COMM_WORLD, &status);
                //Send left.
                MPI_Send(&send_left_data[0], side_length, MPI_INT, left_side, 4, MPI_COMM_WORLD);
                //Send right.
                MPI_Send(&send_right_data[0], side_length, MPI_INT, right_side, 6, MPI_COMM_WORLD);
            }
            // With this if else block all threads make their up-bottom information exchange.
            if(((p_rank - 1) / num_matrix_at_side) % 2 == 0) {
                // Calculate the top side ranks to receive and send the data.
                int top_side = (p_rank - 1) / num_matrix_at_side == 0 ? p_rank + (num_matrix_at_side - 1) * num_matrix_at_side : p_rank - num_matrix_at_side;
                int top_left_side = p_rank % num_matrix_at_side == 1 ? top_side + num_matrix_at_side - 1 : top_side - 1;
                int top_right_side = p_rank % num_matrix_at_side == 0 ? top_side - num_matrix_at_side + 1 : top_side + 1;
                // Calculate the bottom side ranks to receive and send the data.
                int bottom_side = p_rank + num_matrix_at_side;
                int bottom_left_side = p_rank % num_matrix_at_side == 1 ? bottom_side + num_matrix_at_side - 1 : bottom_side - 1;
                int bottom_right_side = p_rank % num_matrix_at_side == 0 ? bottom_side - num_matrix_at_side + 1 : bottom_side + 1;
                // Receive top data.
                MPI_Recv(&receive_top_data[0], side_length, MPI_INT, top_side, 2, MPI_COMM_WORLD, &status);
                // Receive top left data.
                MPI_Recv(&recv_top_left_corner, 1, MPI_INT, top_left_side, 3, MPI_COMM_WORLD, &status);
                // Receive top right data.
                MPI_Recv(&recv_top_right_corner, 1, MPI_INT, top_right_side, 1, MPI_COMM_WORLD, &status);
                // Receive bottom.
                MPI_Recv(&receive_bottom_data[0], side_length, MPI_INT, bottom_side, 8, MPI_COMM_WORLD, &status);
                // Receive bottom left.
                MPI_Recv(&recv_bottom_left_corner, 1, MPI_INT, bottom_left_side, 9, MPI_COMM_WORLD, &status);
                // Receive bottom right.
                MPI_Recv(&recv_bottom_right_corner, 1, MPI_INT, bottom_right_side, 7, MPI_COMM_WORLD, &status);
                // Sending top data.
                MPI_Send(&send_top_data[0], side_length, MPI_INT, top_side, 8, MPI_COMM_WORLD);
                // Sending top left data.
                MPI_Send(&send_top_left_corner, 1, MPI_INT, top_left_side, 7, MPI_COMM_WORLD);
                // Sending top right data.
                MPI_Send(&send_top_right_corner, 1, MPI_INT, top_right_side, 9, MPI_COMM_WORLD);
                // Sending bottom data.
                MPI_Send(&send_bottom_data[0], side_length, MPI_INT, bottom_side, 2, MPI_COMM_WORLD);
                // Sending bottom left data.
                MPI_Send(&send_bottom_left_corner, 1, MPI_INT, bottom_left_side, 1, MPI_COMM_WORLD);
                // Sending bottom right data.
                MPI_Send(&send_bottom_right_corner, 1, MPI_INT, bottom_right_side, 3, MPI_COMM_WORLD);
            } else {
                // Calculate the top side ranks to receive and send the data.
                int top_side = p_rank - num_matrix_at_side;
                int top_right_side = p_rank % num_matrix_at_side == 0 ? top_side - num_matrix_at_side + 1 : top_side + 1;
                int top_left_side = p_rank % num_matrix_at_side == 1 ? top_side + num_matrix_at_side - 1 : top_side - 1;
                // Calculate the bottom side ranks to receive and send the data.
                int bottom_side = (p_rank - 1) / num_matrix_at_side == num_matrix_at_side - 1 ? p_rank - num_matrix_at_side * (num_matrix_at_side - 1) :
                            p_rank + num_matrix_at_side;
                int bottom_right_side = p_rank % num_matrix_at_side == 0 ? bottom_side - num_matrix_at_side + 1 : bottom_side + 1;
                int bottom_left_side = p_rank % num_matrix_at_side == 1 ? bottom_side + num_matrix_at_side - 1 : bottom_side - 1;

                // Sending top data.
                MPI_Send(&send_top_data[0], side_length, MPI_INT, top_side, 8, MPI_COMM_WORLD);
                // Sending top left data.
                MPI_Send(&send_top_left_corner, 1, MPI_INT, top_left_side, 7, MPI_COMM_WORLD);
                // Sending top right data.
                MPI_Send(&send_top_right_corner, 1, MPI_INT, top_right_side, 9, MPI_COMM_WORLD);
                // Sending bottom data.
                MPI_Send(&send_bottom_data[0], side_length, MPI_INT, bottom_side, 2, MPI_COMM_WORLD);
                // Sending bottom left data.
                MPI_Send(&send_bottom_left_corner, 1, MPI_INT, bottom_left_side, 1, MPI_COMM_WORLD);
                // Sending bottom right data.
                MPI_Send(&send_bottom_right_corner, 1, MPI_INT, bottom_right_side, 3, MPI_COMM_WORLD);

                // Receive top data.
                MPI_Recv(&receive_top_data[0], side_length, MPI_INT, top_side, 2, MPI_COMM_WORLD, &status);
                // Receive top left data.
                MPI_Recv(&recv_top_left_corner, 1, MPI_INT, top_left_side, 3, MPI_COMM_WORLD, &status);
                // Receive top right data.
                MPI_Recv(&recv_top_right_corner, 1, MPI_INT, top_right_side, 1, MPI_COMM_WORLD, &status);
                // Receive bottom data.
                MPI_Recv(&receive_bottom_data[0], side_length, MPI_INT, bottom_side, 8, MPI_COMM_WORLD, &status);
                // Receive bottom left data.
                MPI_Recv(&recv_bottom_left_corner, 1, MPI_INT, bottom_left_side, 9, MPI_COMM_WORLD, &status);
                // Receive bottom right data.
                MPI_Recv(&recv_bottom_right_corner, 1, MPI_INT, bottom_right_side, 7, MPI_COMM_WORLD, &status);
            }
            // Code below creates a bigger 2d array to make use of the received data.
            int bigger_current_data[side_length + 2][side_length + 2];
            for (int i = 0; i < side_length; ++i) {
                for (int j = 0; j < side_length; ++j) {
                    bigger_current_data[i+1][j+1] = current_data[i][j];
                }
            }
            // Initialize the values received to the new array
            bigger_current_data[0][0] = recv_top_left_corner;
            bigger_current_data[0][side_length + 1] = recv_top_right_corner;
            bigger_current_data[side_length + 1][0] = recv_bottom_left_corner;
            bigger_current_data[side_length + 1][side_length + 1] = recv_bottom_right_corner;
            for(int i = 0; i < side_length; i++) {
                bigger_current_data[0][i + 1] = receive_top_data[i];
                bigger_current_data[side_length + 1][i + 1] = receive_bottom_data[i];
                bigger_current_data[i + 1][side_length + 1] = receive_right_data[i];
                bigger_current_data[i + 1][0] = receive_left_data[i];
            }

            // Find the next state of the current cell and update the current data in the rank
            for (int i = 1; i < side_length + 1; ++i) {
                for (int j = 1; j < side_length + 1; ++j) {
                    int cell = bigger_current_data[i][j];
                    int cell_count = 0;
                    for (int k = -1; k < 2; ++k) {
                        for (int l = -1; l < 2; ++l) {
                            if(k == 0 && l == 0) {
                                continue;
                            }
                            if(bigger_current_data[i+k][j+l] == 1) {
                                cell_count++;
                            }
                        }
                    }
                    if(cell == 1) {
                        if(cell_count >= 2 && cell_count <= 3) {

                        } else {
                            current_data[i-1][j-1] = 0;
                        }
                    } else {
                        if(cell_count == 3) {
                            current_data[i-1][j-1] = 1;
                        }
                    }
                }
            }
        }
        //If process is not master process it sends its map information to master.
        if(p_rank != 0){
            vector<int> linear_send_data(data_size);
            int count = 0;
            for (int i = 0; i < side_length; ++i) {
                for (int j = 0; j < side_length; ++j) {
                    linear_send_data[count] = current_data[i][j];
                    ++count;
                }
            }
            MPI_Send(&linear_send_data[0], data_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
    MPI_Finalize();
    return 0;
}