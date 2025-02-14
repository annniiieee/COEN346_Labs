// Tried doing multithreading in C++ and it might work?? but it kinda sucks, i dont think the mergesort even works without multithreading sometimes

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
using namespace std;
using namespace std::chrono;

void merge(int arr[], int leftIndex, int midIndex, int rightIndex)
{
    int lSize = midIndex + 1;
    int rSize = rightIndex - midIndex;

    int leftArray[lSize];
    int rightArray[rSize];

    // Copy contents to left array
    for (int i = 0; i < lSize; i++)
        leftArray[i] = arr[i];

    // Copy contents to right array
    for (int j = 0; j < rSize; j++)
        rightArray[j] = arr[midIndex + 1 + j];

    int i = 0, j = 0;
    int k = leftIndex;

    // Compare contents in both sub-arrays and sort in large array
    while (i < lSize && j < rSize)
    {
        if (leftArray[i] <= rightArray[j])
        {
            arr[k] = leftArray[i];
            i++;
            k++;
        }
        else
        {
            arr[k] = rightArray[j];
            j++;
            k++;
        }
    }

    // Insert remaining elements in large array
    while (i < lSize)
    {
        arr[k] = leftArray[i];
        i++;
        k++;
    }
    while (j < rSize)
    {
        arr[k] = rightArray[j];
        j++;
        k++;
    }
}

// Split large array into two sub-arrays
void mergeSort(int arr[], int leftIndex, int rightIndex)
{
    if (leftIndex >= rightIndex)    // Base case
        return;

    int midIndex = leftIndex + (rightIndex - leftIndex) / 2;

    // mergeSort(arr, leftIndex, midIndex);
    // mergeSort(arr, midIndex + 1, rightIndex);

  // Create threads for recursive calls 
    thread t1(mergeSort, arr, leftIndex, midIndex);
    thread t2(mergeSort, arr, midIndex + 1, rightIndex);

  // Join threads
    t1.join();
    t2.join();


    merge(arr, leftIndex, midIndex, rightIndex);
}

void printArray(int arr[], int size)
{
    for (int i = 0; i < size; i++)
        cout << arr[i] <<"\n";
}
int main() {
    const int size = 1000;
    int arr[size];

    int j = 0;
    for (int i = size - 1; i >= 0; i--)
    {
        arr[j] = i;
        j++;
    }

    printArray(arr, size);


    mergeSort(arr,0,size - 1);


    printArray(arr, size);



}
