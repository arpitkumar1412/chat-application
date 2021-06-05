# chat-application
  1. This is a multi-user client-server based chat application/group messaging system. The design features kept in mind while designing the application were - It should be able to allow users in a UNIX system to create groups, list groups, join, send private messages, send message to groups, receive message from groups in online/offline modes. A user may set auto delete <t> option which means, users who joined after t seconds from the time of message creation, to them message will not be delivered.

  2. This was designed using message queues present in UNIX system for interprocess communication.

  3. The design and implementation of the program can be understood from the describe.pdf attached in the repo.

  4. To run the program, run the server.c file first, followed by any number of client.c files. Here, each client acts as a separate user accessing the messaging system. Rest all instruction can be found in the describe.pdf file.
