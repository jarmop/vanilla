// Deno.serve(
//   {
//     port: 443,
//     cert: Deno.readTextFileSync("./cert.pem"),
//     key: Deno.readTextFileSync("./key.pem"),
//   },
//   (_req) => {
//     return new Response("Hello, World!");
//   },
// );

const listener = Deno.listenTls({
  hostname: "localhost",
  // hostname: "127.0.0.1",
  // keyFormat: "pem",
  port: 8443,
  transport: "tcp",
  // cert: Deno.readTextFileSync("./server.crt"),
  // key: Deno.readTextFileSync("./server.key"),
  cert: Deno.readTextFileSync("../../cert.pem"),
  key: Deno.readTextFileSync("../../key.pem"),
  // certFile: "./cert.pem",
  // keyFile: "./key.pem",
});
for await (const conn of listener) {
  await handle(conn);
  try {
    await conn.readable.pipeTo(Deno.stdout.writable);
  } catch (err) {
    // console.log("eukryu");
    //   // console.log("------START------");
    //   // console.log(err);
    //   // console.log("------END------");
  }

  conn.close();
}

async function handle(conn: Deno.TlsConn) {
  try {
    // Read the HTTP request (not robust, but minimal)
    const buf = new Uint8Array(1024);
    await conn.read(buf);

    // Send minimal valid HTTP response
    const response = "HTTP/1.1 200 OK\r\n" +
      "Content-Type: text/plain\r\n" +
      "Content-Length: 13\r\n" +
      "\r\n" +
      "Hello, World!";

    await conn.write(new TextEncoder().encode(response));
  } finally {
    console.log("eukryu");

    conn.close();
  }
}
