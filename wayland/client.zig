const std = @import("std");
const print = std.debug.print;
const linux = std.os.linux;
const Init = std.process.Init;
const Environ = std.process.Environ;

const display_id: u32 = 1;

fn connectServer(env_map: *Environ.Map) !i32 {
    // zig implementation of linux.socket return usize which matches the address space of the system,
    // so 64-bit in my case, so need to cast to 32-bit int which is expected by the connect syscall.
    const fd: i32 = @intCast(linux.socket(linux.AF.UNIX, linux.SOCK.STREAM | linux.SOCK.CLOEXEC, 0));

    var path: [108]u8 = @splat(0);
    const runtime = env_map.get("XDG_RUNTIME_DIR").?;
    const display = env_map.get("WAYLAND_DISPLAY").?;
    const path_len = runtime.len + 1 + display.len;
    @memcpy(path[0..runtime.len], runtime);
    @memcpy(path[runtime.len..(runtime.len + 1)], "/");
    @memcpy(path[(runtime.len + 1)..(path_len)], display);
    const addr: linux.sockaddr.un = .{ .family = linux.AF.UNIX, .path = path };
    // print("{s}\n", .{path});

    // For some reason, can't cast directly to i32, but has to cast first to integer of usize
    // and then cast that to the integer size required by the connect function
    const connectError64: i64 = @bitCast(linux.connect(fd, &addr, @sizeOf(linux.sockaddr.un)));
    const connectError32: i32 = @intCast(connectError64);
    if (connectError32 < 0) {
        print("Connect error: {}\n", .{connectError32});
        std.os.linux.exit(1);
    }

    return fd;
}

const Header = packed struct { object_id: u32, opcode: u16, size: u16 };

inline fn toBytes(val: anytype) []const u8 {
    // array literal requires address-of operator (&) to coerce to slice type '[]const u8'
    return &@as([@sizeOf(@TypeOf(val))]u8, @bitCast(val));
}

fn getRegistry(fd: i32, id: u32) void {
    const header: Header = .{ .object_id = display_id, .opcode = 1, .size = 12 };
    const headerBytes = toBytes(header);

    const GetRegistryPayload = packed struct { registry: u32 };
    const payload: GetRegistryPayload = .{ .registry = id };
    const payloadBytes = toBytes(payload);

    var msg: [headerBytes.len + @sizeOf(GetRegistryPayload)]u8 = undefined;
    @memcpy(msg[0..headerBytes.len], headerBytes);
    @memcpy(msg[headerBytes.len..msg.len], payloadBytes);

    // const msg = [_]u8{ 1, 0, 0, 0, 1, 0, 12, 0, 2, 0, 0, 0 };

    _ = linux.write(fd, &msg, msg.len);
    // _ = linux.write(fd, @ptrCast(headerBytes), @sizeOf(Header));
    // _ = linux.write(fd, @ptrCast(payloadBytes), @sizeOf(GetRegistryPayload));

    // print("Wrote {} bytes\n", .{n});
}

fn printBytes(arr: []const u8) void {
    for (arr) |value| {
        print("{x}|", .{value});
    }
    print("\n", .{});
}

pub fn main(init: Init) !void {
    const fd = try connectServer(init.environ_map);
    // print("fd: {}\n", .{fd});

    const registry_id: u32 = 2;
    getRegistry(fd, registry_id);

    var i: u32 = 0;
    while (true) : (i += 1) {
        var header: Header = undefined;
        _ = linux.read(fd, @ptrCast(&header), @sizeOf(Header));
        const payload_size = header.size - @sizeOf(Header);

        // print("Read {} bytes – Payload size: {}\n", .{ n, payload_size });

        var payload: [4096]u8 = undefined;
        _ = linux.read(fd, &payload, payload_size);

        if (header.object_id == display_id) {
            const error_object_id = std.mem.readInt(u32, payload[0..4], .little);
            const error_code = std.mem.readInt(u32, payload[4..8], .little);
            const error_msg_len = std.mem.readInt(u32, payload[8..12], .little);
            const error_msg = payload[12 .. 12 + error_msg_len];
            print("Error – object_id: {}, error_code: {}, error_msg: {s}\n", .{ error_object_id, error_code, error_msg });
        } else if (header.object_id == registry_id) {
            // numeric name of the global object
            const name = std.mem.readInt(u32, payload[0..4], .little);
            // interface implemented by the object
            const interface = payload[4..(payload_size - 4)];
            // -1 to ignore the null delimiter as zig strings don't have those
            const interface_name_len = std.mem.readInt(u32, interface[0..4], .little) - 1;
            // const interface_name_len = std.mem.readInt(u32, interface[0..4], .little);
            const interface_name = interface[4..(4 + interface_name_len)];
            // interface version
            const version = std.mem.readInt(u32, payload[(payload_size - 4)..payload_size][0..4], .little);
            print("{}\tv{}\t{s},\n", .{ name, version, interface_name });

            // printBytes(interface_name);
            // printBytes("wl_compositor");

            // const foo = std.mem.find(u8, interface_name, "wl_compositor") orelse 1;

            if (std.mem.eql(u8, interface_name, "wl_compositor")) {
                // if (@bitCast(@as(u1, @intCast(std.mem.find(u8, interface_name, "wl_compositor").?)))) {
                // if (std.mem.find(u8, interface_name, "wl_compositor").? > 0) {
                // if (foo == 0) {
                print("match\n", .{});
                const strlen: u32 = 14;
                // +2 to align by four bytes
                var str: [4 + strlen + 2]u8 = @splat(0);
                // const prkl = [_]u8{0};
                @memcpy(str[0..4], toBytes(strlen));
                @memcpy(str[4 .. 4 + (strlen - 1)], interface_name);
                // @memcpy(str[(str.len - 1)..str.len], &prkl);
                // const BindPayload = packed struct { name: u32, interface: [18]u8, version: u32, new_id: u32 };
                // const bind_payload: BindPayload = .{ .name = name, .interface = str, .version = version, .new_id = 3 };
                // const bind_payload_bytes = toBytes(bind_payload);
                const bind_payload_size = 4 + 20 + 4 + 4;
                var bind_payload_bytes: [bind_payload_size]u8 = undefined;
                const wl_compositor_id: u32 = 3;
                @memcpy(bind_payload_bytes[0..4], toBytes(name));
                @memcpy(bind_payload_bytes[4..(4 + str.len)], &str);
                @memcpy(bind_payload_bytes[(4 + str.len)..(4 + str.len + 4)], toBytes(version));
                @memcpy(bind_payload_bytes[(4 + str.len + 4)..bind_payload_size], toBytes(wl_compositor_id));

                const bind_header: Header = .{ .object_id = registry_id, .opcode = 0, .size = @sizeOf(Header) + bind_payload_size };
                const bind_header_bytes = toBytes(bind_header);

                var msg: [bind_header_bytes.len + bind_payload_size]u8 = undefined;
                @memcpy(msg[0..bind_header_bytes.len], bind_header_bytes);
                @memcpy(msg[bind_header_bytes.len..msg.len], &bind_payload_bytes);

                printBytes(&msg);

                const n = linux.write(fd, &msg, msg.len);
                print("Wrote {}\n", .{n});
            }
        }
    }
}
