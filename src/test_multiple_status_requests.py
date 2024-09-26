
import os
import time

# Path to the server's FIFO (replace with actual path to orchestrator's server FIFO)
SERVER_FIFO = "/tmp/server_fifo"

# Simulate multiple clients sending "status" requests
def send_status_request(client_id):
    try:
        # Each client has a unique FIFO for reading responses from the server (replace with actual path)
        client_fifo = f"/tmp/client_fifo_{client_id}"
        os.mkfifo(client_fifo)  # Create client-specific FIFO
        
        # Send "status" request to the server
        with open(SERVER_FIFO, 'w') as server_fifo:
            server_fifo.write(f"status {client_id}\n")
            server_fifo.flush()

        # Read the server's response from the client FIFO
        with open(client_fifo, 'r') as client_fifo_read:
            status_output = client_fifo_read.read()
            print(f"Client {client_id} received status: {status_output}")
        
    finally:
        # Clean up the client FIFO
        if os.path.exists(client_fifo):
            os.remove(client_fifo)

# Simulate 5 clients requesting status in parallel
if __name__ == "__main__":
    num_clients = 5
    processes = []

    for i in range(num_clients):
        pid = os.fork()
        if pid == 0:
            # Child process (individual client)
            send_status_request(i)
            os._exit(0)
        else:
            # Parent process
            processes.append(pid)
    
    # Wait for all child processes to finish
    for process in processes:
        os.waitpid(process, 0)
