# threading module is used to create threads for parallel execution
import threading 

# Merge two sorted arrays into a single sorted array
# params: left, right - two sorted arrays
def merge(left, right):
    result = []
    i = j = 0
    while i < len(left) and j < len(right):
        if left[i] < right[j]:
            result.append(left[i])
            i += 1
        else:
            result.append(right[j])
            j += 1
    # Append the remaining elements of left and right, if any
    result.extend(left[i:])
    result.extend(right[j:])
    return result

# MergeSortThread class is a subclass of the threading.Thread class
class MergeSortThread(threading.Thread):
    # lock to ensure that the log is written in a thread-safe manner
    log_lock = threading.Lock()

    # Input added as an attribute of the class
    def __init__(self, arr, thread_head, output_log):
        super().__init__()
        self.arr = arr
        self.thread_head = thread_head
        self.output_log = output_log
        self.sorted_arr = []

    # run method called when thread is startd 
    def run(self):
        with MergeSortThread.log_lock:
            self.output_log.append(f"Thread {self.thread_head} started")

        # Base case: if array has only one element, return as is
        if len(self.arr) <= 1:
            self.sorted_arr = self.arr  # Ensure the single-element array is returned
        else:
            mid = len(self.arr) // 2

            # append 0 to the thread_head for the left thread and 1 for the right thread
            left_thread = MergeSortThread(self.arr[:mid], self.thread_head + "0", self.output_log)
            right_thread = MergeSortThread(self.arr[mid:], self.thread_head + "1", self.output_log)

            # thread execution order is managed by the operating system (OS) scheduler, 
            # which does not guarantee a fixed execution sequence unless explicitly controlled.
            left_thread.start()
            right_thread.start()

            # join method is used to wait for the threads to finish
            left_thread.join()
            right_thread.join()

            self.sorted_arr = merge(left_thread.sorted_arr, right_thread.sorted_arr)
    
        with MergeSortThread.log_lock:
            # appending a map since the sorted_arr is a list of integers
            self.output_log.append(f"Thread {self.thread_head} finished: {', '.join(map(str, self.sorted_arr))}")

if __name__ == "__main__":
    # Read input line by line from input.txt
    print ("Reading input from input.txt")
    with open("input.txt", "r") as file:
        numbers = [int(line.strip()) for line in file]
    
    output_log = []
    main_thread = MergeSortThread(arr=numbers, thread_head="1", output_log=output_log)
    main_thread.start()
    main_thread.join()
    
    print ("Writing output to output.txt")
    with open("output.txt", "w") as file:
        file.write("\n".join(output_log) + "\n")