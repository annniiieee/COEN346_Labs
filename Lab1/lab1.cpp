#include <iostream>
#include <thread>
#include <string>
#include <fstream>
#include <vector>

using namespace std;

// Function to merge two sorted subarrays
void merge(vector<int>& arr, int left, int mid, int right, string id, ofstream &outFile) {

    vector<int> temp; // Creating a temporary vector to store the sorted array

    int i = left; // Initialize the pointers for the left subarray
    int j = mid + 1; // Initialize the pointers for the right subarray

    // Compare the elements in the left and right subarrays and push the smaller element to the temp vector
    while (i <= mid && j <= right) {
        if (arr[i] <= arr[j]) {
            temp.push_back(arr[i++]); // If arr[i] is smaller, push it to the temp vector
        }
        else {
            temp.push_back(arr[j++]); // If arr[j] is smaller, push it to the temp vector
        }
    }
    // Push the remaining elements in the left subarray to the temp vector
    while (i <= mid) temp.push_back(arr[i++]);

    // Push the remaining elements in the right subarray to the temp vector
    while (j <= right) temp.push_back(arr[j++]);

    // Copy the sorted elements from the temp vector to the original array
    for (int k = 0; k < temp.size(); k++) {
        arr[left + k] = temp[k];
    }

    // Write the result to the output file
    outFile << "Thread " << id << " finished: ";
    for (int i = left; i <= right; i++) { // Write the sorted array to the output file
        outFile << arr[i] << (i == right ? ",\n" : " ");
    }
}

// Multithreaded mergeSort function
void mergeSort(vector<int>& arr, int left, int right, string id, ofstream &outFile) {

    // Base case reached when left index = right index
    if (left >= right) {
        outFile << "Thread " << id << " finished: " << arr[left] << ",\n";
        return;
    }

    // Calculate the middle index
    int mid = left + (right - left) / 2;

    // Write the start of the thread to the output file
    outFile << "Thread " << id << " started\n";

    // Create the id for the left and right threads, respectively
    string leftId = id + '0';
    string rightId = id + '1';

    // Create both threads first before joining
    thread leftThread(mergeSort, ref(arr), left, mid, leftId, ref(outFile));
    thread rightThread(mergeSort, ref(arr), mid + 1, right, rightId, ref(outFile));

    // Join the threads
    leftThread.join();
    rightThread.join();

    // After both threads finish, merge the results
    merge(arr, left, mid, right, id, outFile);

    // Write the result to the output file
    outFile << "Thread " << id << " finished: ";
    for (int i = left; i <= right; i++) { // Write the sorted array to the output file
        outFile << arr[i] << (i == right ? ",\n" : " ");
    }
}

// Driver function
int main() {

    // Open the input and output files, respectively
    ifstream inFile("Input.txt");
    ofstream outFile("Output.txt");

    // Check if the files are opened successfully
    if (!inFile || !outFile) {
        cerr << "Error opening file!" << endl;
        return 1;
    }

    // Initialize the vector and the integer variable
    vector<int> arr;
    int num;

    // Read the input file
    while (inFile >> num) { // Read the numbers from the input file
        arr.push_back(num); // Push the number to the vector 'arr'
    }

    int n = arr.size(); // Get the size of the vector 'arr'

    // Use multithreaded mergesort to sort the array, and write the result to the output file
    mergeSort(arr, 0, n - 1, "1", outFile);

    // Close the input and output files, respectively
    inFile.close();
    outFile.close();

    return 0;
}
