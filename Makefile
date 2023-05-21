define confirm
	@read -p "Are you sure you want to run this command? [y/N] " answer; \
	if [ "$$answer" != "y" -a "$$answer" != "Y" ]; then \
			echo "Aborted."; exit 1; \
	fi
endef

all:
	gcc -Wall -pthread client.c fil.c functions.c -o client
	gcc -Wall -pthread server.c fil.c functions.c -o server

clean:
	rm -rf server
	rm -rf client

reset_database:
	$(call confirm)
	rm -rf client_id.data
	rm -rf server_users.data
	rm -rf infos.data
	rm -rf address.data
	rm -rf fil[0-9]*