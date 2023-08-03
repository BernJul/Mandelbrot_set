#include "asg2.h"
#include <stdio.h>
#include <mpi.h>


static int rank;
static int world_size;

void MPICompute(Point *my_elements, const int num_of_elements) {

	/* compute for all points one by one */
	for (size_t index = 0; index < num_of_elements; index++) {
		compute(my_elements);
		my_elements++;
	}
}

int main(int argc, char *argv[]) {
	if ( argc == 4 ) {
		X_RESN = atoi(argv[1]);
		Y_RESN = atoi(argv[2]);
		max_iteration = atoi(argv[3]);
	} else {
		X_RESN = 1000;
		Y_RESN = 1000;
		max_iteration = 100;
	}

	if (rank == 0) {
		#ifdef GUI
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
		glutInitWindowSize(500, 500); 
		glutInitWindowPosition(0, 0);
		glutCreateWindow("MPI");
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glMatrixMode(GL_PROJECTION);
		gluOrtho2D(0, X_RESN, 0, Y_RESN);
		glutDisplayFunc(plot);
		#endif
	}

	/* computation part begin */
	MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	

	total_size = X_RESN * Y_RESN;

	if (rank == 0){
		initData();
	}

	/* Creating MPI Datatype for the point struct */
	/* Creating helper variables */
	Point helper;
	helper.x = 1;
	helper.y = 2;
	helper.color = 2.0;

	/* Set argument variables for MPI_Type_create_struct */
	const int num_fields = 3;
	MPI_Aint mpi_disp[num_fields];
	const int blocklens[] = {1, 1, 1};
	MPI_Datatype types[] = {MPI_INT, MPI_INT, MPI_FLOAT};
	MPI_Aint start_address;
	MPI_Aint address;

	/* Calculating the displacement size */
	MPI_Get_address(&helper.x, &start_address);
	mpi_disp[0] = 0;

	MPI_Get_address(&helper.y, &address);
	mpi_disp[1] = address - start_address;

	MPI_Get_address(&helper.color, &address);
	mpi_disp[2] = address - start_address;

	/* Creating the MPI struct */
	MPI_Datatype MPI_POINT;
	MPI_Type_create_struct(num_fields, blocklens, mpi_disp, types, &MPI_POINT);
	MPI_Type_commit(&MPI_POINT);

	/* Start timer */
	if (rank == 0) t1 = std::chrono::high_resolution_clock::now();

	/* Scattering data from master processs to all other processes via MPI_Scatterv */
	int *scount = new int[world_size]; // array to hold number of elements to send to each process in order
	int *displacement = new int[world_size]; // array to hold the displacement between data in the array to scatter to each process
	int remainder = total_size % world_size; // remaider of data

	/* number of elements of each process, remainder of elements is allocated to processes at the start in order of rank 0, 1, ... */
	int num_my_elements = (rank < remainder) ? (total_size / world_size) + 1 : (total_size / world_size);
	Point *my_elements = new Point[num_my_elements];

	/* Setup the configs and args for MPI_Scatterv */
    int temp; // helper variable to store the sum of scount[i] for displaement
    for (size_t i = 0; i < world_size; i++) {
        scount[i] = (i < remainder) ? total_size / world_size + 1 : total_size / world_size;
        displacement[i] = temp;
        temp += scount[i]; 
    }

	MPI_Barrier(MPI_COMM_WORLD);
	
	/* Scatter data */
	try {	
		MPI_Scatterv(data, scount, displacement, MPI_POINT, my_elements, num_my_elements, MPI_POINT, 0, MPI_COMM_WORLD);

	} catch(const std::exception& e) {std::cerr << e.what() << std::endl;}

	/* All processes compute for the Madelbrot Set */
	MPICompute(my_elements, num_my_elements);

	/* Gather data from all processes to master process following MPI_Scatterv arguments */
    try {
        MPI_Gatherv(my_elements, num_my_elements, MPI_POINT, data, scount, displacement, MPI_POINT, 0, MPI_COMM_WORLD); // collect result from each process

    } catch(const std::exception& e) {std::cerr << e.what() << std::endl;}

	delete[] my_elements;
	delete[] scount;
	delete[] displacement;

	if (rank == 0){
		t2 = std::chrono::high_resolution_clock::now();  
		time_span = t2 - t1;
	}

	if (rank == 0){
		printf("Student ID: 119010520\n"); // replace it with your student id
		printf("Name: Bernaldy Jullian\n"); // replace it with your name
		printf("Assignment 2 MPI\n");
		printf("Run Time: %f seconds\n", time_span.count());
		printf("Problem Size: %d * %d, %d\n", X_RESN, Y_RESN, max_iteration);
		printf("Process Number: %d\n", world_size);
	}

	/* computation part end */

	if (rank == 0){
		#ifdef GUI
		glutMainLoop();
		#endif
	}

	delete[] data;
	MPI_Finalize();
	return 0;
}