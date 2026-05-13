/*
 * leak_test_cpp.cpp - C++ language memory leak test program
 *
 * This program demonstrates various types of memory leaks:
 * 1. new without delete
 * 2. new[] without delete[]
 * 3. Exception causing leak
 * 4. STL container leak
 * 5. Shared pointer cycle
 */

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstring>
#include <unistd.h>

#define ALLOC_SIZE 1024
#define LOOP_COUNT 100
#define SLEEP_INTERVAL 1

static volatile bool running = true;

void simple_leak(void)
{
	int *ptr = new int(42);
	std::cout << "[simple_leak] allocated int at " << ptr 
	          << " value=" << *ptr << " (never deleted)" << std::endl;
}

void array_leak(void)
{
	int *arr = new int[100];
	for (int i = 0; i < 100; i++) {
		arr[i] = i * 2;
	}
	std::cout << "[array_leak] allocated int[100] at " << arr 
	          << " (never deleted)" << std::endl;
}

void loop_leak(void)
{
	for (int i = 0; i < 10; i++) {
		double *ptr = new double[ALLOC_SIZE];
		for (int j = 0; j < ALLOC_SIZE; j++) {
			ptr[j] = j * 0.5;
		}
		std::cout << "[loop_leak] iteration " << i 
		          << ": allocated double[" << ALLOC_SIZE << "] at " << ptr << std::endl;
	}
}

void object_leak(void)
{
	class TestObject {
	public:
		int id;
		char data[256];
		TestObject(int i) : id(i) {
			memset(data, 0, 256);
		}
	};

	TestObject *obj = new TestObject(123);
	std::cout << "[object_leak] allocated TestObject at " << obj 
	          << " id=" << obj->id << " (never deleted)" << std::endl;
}

void vector_leak(void)
{
	std::vector<int*> *vec = new std::vector<int*>;
	for (int i = 0; i < 20; i++) {
		int *elem = new int(i * 10);
		vec->push_back(elem);
	}
	std::cout << "[vector_leak] allocated vector with " << vec->size() 
	          << " int pointers at " << vec << std::endl;

	for (auto ptr : *vec) {
		delete ptr;
	}
	delete vec;
	std::cout << "[vector_leak] properly cleaned up" << std::endl;
}

void vector_pointer_leak(void)
{
	std::vector<int*> vec;
	for (int i = 0; i < 20; i++) {
		vec.push_back(new int(i * 10));
	}
	std::cout << "[vector_pointer_leak] vector holds " << vec.size() 
	          << " leaked int pointers" << std::endl;
}

void map_leak(void)
{
	std::map<std::string, int*> *m = new std::map<std::string, int*>;
	for (int i = 0; i < 10; i++) {
		std::string key = "key_" + std::to_string(i);
		int *val = new int(i * 100);
		m->insert({key, val});
	}
	std::cout << "[map_leak] allocated map with " << m->size() 
	          << " entries at " << m << std::endl;
}

void exception_leak(void)
{
	int *ptr = new int(999);
	std::cout << "[exception_leak] allocated int at " << ptr << std::endl;
	
	try {
		throw std::runtime_error("Simulated exception");
	} catch (const std::exception& e) {
		std::cout << "[exception_leak] caught exception: " << e.what() 
		          << ", ptr at " << ptr << " leaked" << std::endl;
	}
}

void nested_object_leak(void)
{
	struct Node {
		int value;
		Node *left;
		Node *right;
		Node(int v) : value(v), left(nullptr), right(nullptr) {}
	};

	Node *root = new Node(1);
	root->left = new Node(2);
	root->right = new Node(3);
	root->left->left = new Node(4);
	root->right->right = new Node(5);

	std::cout << "[nested_object_leak] created tree structure, root at " << root << std::endl;

	delete root->left->left;
	delete root->right->right;
	std::cout << "[nested_object_leak] deleted only leaf nodes, rest leaked" << std::endl;
}

void string_leak(void)
{
	char *str = new char[256];
	strcpy(str, "This is a dynamically allocated string");
	std::cout << "[string_leak] allocated string '" << str << "' at " << str 
	          << " (never deleted)" << std::endl;
}

void large_block_leak(void)
{
	char *block = new char[ALLOC_SIZE * 50];
	memset(block, 'X', ALLOC_SIZE * 50);
	std::cout << "[large_block_leak] allocated " << ALLOC_SIZE * 50 
	          << " bytes at " << block << std::endl;
}

int main(int argc, char *argv[])
{
	std::cout << "=== Memory Leak Test Program (C++ Version) ===" << std::endl;
	std::cout << "Run with: sudo ./build_output/hub/memleak/memleak -p $(pidof leak_test_cpp)" << std::endl;
	std::cout << "Press Ctrl+C to stop" << std::endl << std::endl;

	int iterations = 0;

	while (running) {
		std::cout << std::endl << "--- Iteration " << iterations + 1 << " ---" << std::endl;

		simple_leak();
#if 0
		array_leak();
		loop_leak();
		object_leak();
		vector_leak();
		vector_pointer_leak();
		map_leak();
		exception_leak();
		nested_object_leak();
		string_leak();
		large_block_leak();
#endif

		sleep(SLEEP_INTERVAL);
		iterations++;

		if (iterations >= LOOP_COUNT) {
			std::cout << std::endl << "Reached " << LOOP_COUNT << " iterations, continuing..." << std::endl;
		}
	}

	std::cout << std::endl << "Program terminated (memory intentionally leaked)" << std::endl;
	return 0;
}
