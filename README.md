The Synthesizer is created using three main libraries <portaudio> <SDL2> and <pthread>
Use the below to compile the code :
              gcc cpiano.c -o gui.exe -std=c17 -Dmain=SDL_main -lmingw32 -lSDL2main -lSDL2 -lportaudio -lpthread -lm
This to execute the code:
              ./gui.exe

Sample inputs :
Enter number of keys: 14
Enter keys (1-36 for notes C2 to C7):  
Enter quantum time (seconds per note): 0.5
Select waveform type (1: Sine, 2: Sawtooth, 3: Square, 4: Triangle): 1

Enter number of keys: 25
Enter keys (1-36 for notes C2 to C7): 15 15 17 15 20 19 15 15 17 15 22 20 15 15 17 24 20 19 17 15 15 24 20 22 20
Enter quantum time (seconds per note): 0.4
Select waveform type: 1

Enter number of keys: 11
Enter keys (1-36 for notes C2 to C7): 15 17 19 20 22 24 22 20 19 17 15
Enter quantum time (seconds per note): 0.25
Select waveform type: 1
