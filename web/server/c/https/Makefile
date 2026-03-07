default: build_https run_https

build_https:
	@gcc https.c -lssl -lcrypto

run_https: build_https
	@./a.out