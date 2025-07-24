# Word Party
Word Party is an online multiplayer game about guessing words containing a certain substring of 2 or 3 letters. The timer gradually decreases as more players enter correct words, until someone fails to do so and loses a heart. The last player standing is the winner!
This game was inspired by JKLM's Bomb Party.

![game preview](Preview_img3.png)

## Download
[Download here](https://github.com/dxia2p/Word-Party/tags)

## How do I run the game?
Note: This game only works on linux at the moment\
Run `./server` in your terminal followed by `./client`. Enter the IP address of the computer the server is running on and enter "8080" for the port. If you're playing with people over the internet you may need to port forward.

## How its made:
- This project was made from scratch in C with the built-in socket API, POSIX threads, and select for multiplexing.
- Functions needed by both the server and client are stored in the "public" folder, such as sending/receiving logic and a parser for the custom protocol they use
  - The protocol begins with a byte denoting what type of message it is, all subsequent data is a part of the body
  - "Values" in the body are delimited by the '&' symbol and the message is terminated by '$'
  - Each message code (the byte at the beginning) corresponds to a format string which is used by the sender and receiver to format their data
- The server uses a message queue (made with c11's atomics) to send data between the networking thread and the game logic thread

## Lessons Learned:
- This was my first relatively large project using C, and I learned a lot about project organization and networking concepts
- I realized that breaking the program apart into modules helped immensly with keeping it maintainable
- I learned how abstract data types like sets and queues work under the surface, and implemented them with encapsulation in C
- I also learned that splitting the networking code into its own thread can greatly improve performance compared to polling the network
