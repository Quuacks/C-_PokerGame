# C-_PokerGame

A multiplayer poker game developed in C++ as a collaborative team project.
This project was designed to simulate a functional poker experience with both client-side gameplay and server-side networking/backend systems.

## Features

* Multiplayer poker gameplay
* Client-server architecture
* Real-time game state synchronization
* Player actions (bet, fold, call, raise, etc.)
* Turn-based game flow
* Card and hand management
* Server-side game logic
* Custom UI/client interface

---

## Technologies Used

* **Language:** C++
* **Networking:** Winsock
* **Architecture:** Client/Server
* **Version Control:** Git & GitHub

---

## Project Structure

```bash
C-_PokerGame/
│
├── Client/        # Client-side game logic and UI
├── Server/        # Backend/server logic
├── Shared/        # Shared utilities and data structures
└── README.md
```

---

## Gameplay Overview

This project implements a poker game where multiple players can connect to a server and participate in matches in real time. The server manages game state, player actions, card distribution, and overall match flow, while the clients handle rendering and user interaction.

---

# Team Contributions

## Team Member 1 Armantas Stanevičius (Quuacks - Backend and server)

### Contributions

* Worked on the server. People can connect to the server to play the game. But for now its only local host
* Implemented a request handler for the server so when the server recieves a request it handles it cleanly. Packages are formated to a json using a library nlohmann/json.hpp converted to a string and sent to server or client
* Implemented game logic. Things like creating a deck, shuffling it, dealing cards, all the actions (Call, Raise, Fold, Check).
---

## Team Member 2 Mykolas Liandzbergas (Frontend and client UI)

### Contributions

* Worked on the client-side UI and poker table interface.
* Implemented the raise slider with manual amount input, confirm, and cancel functionality.
* Added a status/message log for displaying player actions, server messages, login status, and game events.
* Implemented chip rendering for player chip stacks, the main pot, and raise amount preview.
* Improved player positioning around the table with player names, chip amounts, and chip stacks.
* Added a “Your Turn” popup and highlighted action buttons when it is the player’s turn.
* Disabled Raise, Call, Check, and Fold buttons when it is not the player’s turn.
* Added a username login window and connected the entered username to the login request.
* Connected UI buttons to the server by sending JSON requests for Raise, Call, Check, and Fold.
* Added a Start Game button that sends a request to the server before a hand starts.
  ---

## Team Member 3

### Contributions

*
*
*
*
*

---

---

## Future Improvements

* Add a Room system where players join rooms to start the game
* Improve UI/UX
* Add persistent accounts/database support
* Improve hand evaluation system

---

## License

This project was created for educational purposes.
