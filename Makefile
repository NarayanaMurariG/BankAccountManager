clean:
	rm Server
	rm Client
	rm MultipleClient

compile:
	g++ Server.cpp -o Server -lpthread
	g++ Client.cpp -o Client
	g++ MultipleClient.cpp -o MultipleClient -lpthread
