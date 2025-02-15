#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
using namespace std;
using namespace std::chrono;

void merge(int arr[], int leftIndex, int midIndex, int rightIndex)
{
    int lSize = midIndex - leftIndex + 1;
    int rSize = rightIndex - midIndex;

    int leftArray[lSize];
    int rightArray[rSize];

    // Copy contents to left array
    for (int i = 0; i < lSize; i++)
        leftArray[i] = arr[leftIndex + i];

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

//    mergeSort(arr, leftIndex, midIndex);
//    mergeSort(arr, midIndex + 1, rightIndex);

    thread t1(mergeSort, arr, leftIndex, midIndex);
    thread t2(mergeSort, arr, midIndex + 1, rightIndex);

    t1.join();
    t2.join();

    merge(arr, leftIndex, midIndex, rightIndex);
}

void printArray(int arr[], int size)
{
    for (int i = 0; i < size; i++)
        cout << arr[i] <<"\n";
}
int main()
{
    const int size = 10000;
    int arr[size];

    // Create random values
    for (int i = size - 1; i >= 0; i--)
        arr[i] = rand() % size;


    // Mergesort execution and timer
    auto t1 = high_resolution_clock::now();
    mergeSort(arr,0,size - 1);
    auto t2 = high_resolution_clock::now();

    cout << duration<double, std::milli>(t2 - t1).count() << "ms\n";
}
