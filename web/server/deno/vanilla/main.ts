import * as denoPath from "@std/path";

const NEWLINE = "\r\n";
const HEADER_BODY_SEPARATOR = NEWLINE + NEWLINE;
const decoder = new TextDecoder();
const encoder = new TextEncoder();
const listener = Deno.listen({
  hostname: "127.0.0.1",
  port: 3000,
  transport: "tcp",
});
for await (const conn of listener) {
  await handleConnection(conn);
  // await conn.readable.pipeTo(Deno.stdout.writable);

  conn.close();
}

async function handleConnection(conn: Deno.TcpConn) {
  console.log(`\n----------------------`);
  console.log(`${(new Date()).toLocaleTimeString("de-DE")}\n`);

  const requestBuffer = new Uint8Array(1024);
  const numberOfBytesRead = await conn.read(requestBuffer);
  if (numberOfBytesRead === null) {
    return;
  }
  // Remove empty bytes
  const trimmedRequestBuffer = requestBuffer.slice(0, numberOfBytesRead);

  const [header, body] = decoder
    .decode(trimmedRequestBuffer)
    .split(
      HEADER_BODY_SEPARATOR,
    );
  // console.log(numberOfBytesRead);
  // console.log(header);
  // console.log(JSON.stringify(body));

  const requestTarget = parseHeader(header);
  const filename = requestTarget.length > 1 ? requestTarget : "index.html";
  const filePath = denoPath.join("../../public/", filename);
  try {
    const responseText = await Deno.readTextFile(filePath);
    // console.log(JSON.stringify(responseText));
    conn.write(encoder.encode(getHeader() + responseText));
  } catch (error) {
    if (error instanceof Deno.errors.NotFound) {
      console.info(`\n%cFile not found: ${filename}`, "color:red");
    } else {
      throw error;
    }
  }
}

function parseHeader(header: string) {
  const requestLine = header.split(NEWLINE)[0];

  // console.log(requestLine);
  console.log(header);

  return requestLine.split(" ")[1];
}

function getHeader() {
  return "HTTP/1.1 200 OK\r\n" +
    "Content-Type: text/html\r\n" +
    "\r\n";
}
