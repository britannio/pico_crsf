void main() {
  var str = StringBuffer();
  for (var payload in Payload.values) {
    payload.toStruct(str);
    str.writeln();
    str.writeln();
  }

  Payload.toTelemetryStruct(str);
  str.writeln();
  str.writeln();

  Payload.toEnum(str);
  str.writeln();
  str.writeln();

  for (var payload in Payload.values) {
    payload.toCBuffer(str);
    str.writeln();
    str.writeln();
  }
  print(str.toString());
}

extension on int {
  String get hex {
    return "0x" + "${toRadixString(16).padLeft(2, '0')}".toUpperCase();
  }
}

enum CType {
  uint8("uint8_t", "buf_write_ui8", 8),
  uint16("uint16_t", "buf_write_ui16", 16),
  uint24("uint24_t", "buf_write_ui24", 24),
  uint32("uint32_t", "buf_write_ui32", 32),
  int8("int8_t", "buf_write_i8", 8),
  int16("int16_t", "buf_write_i16", 16),
  int32("int32_t", "buf_write_i32", 32),
  uint11LE("unsigned", "buf_write_ui11LE", 11, packed: true);

  const CType(this.str, this.writeStr, this.bits, {this.packed = false});

  final String str;
  final String writeStr;
  final int bits;
  final bool packed;

  String toString() => str;
}

class PayloadField {
  final CType type;
  final String name;
  final String description;
  final CType writeType;

  const PayloadField(this.type, this.name, this.description, {CType? writeType})
      : writeType = writeType ?? type;
}

enum Payload {
  rc_channels_packed(
    0x16,
    [
      PayloadField(CType.uint11LE, "channel0", ""),
      PayloadField(CType.uint11LE, "channel1", ""),
      PayloadField(CType.uint11LE, "channel2", ""),
      PayloadField(CType.uint11LE, "channel3", ""),
      PayloadField(CType.uint11LE, "channel4", ""),
      PayloadField(CType.uint11LE, "channel5", ""),
      PayloadField(CType.uint11LE, "channel6", ""),
      PayloadField(CType.uint11LE, "channel7", ""),
      PayloadField(CType.uint11LE, "channel8", ""),
      PayloadField(CType.uint11LE, "channel9", ""),
      PayloadField(CType.uint11LE, "channel10", ""),
      PayloadField(CType.uint11LE, "channel11", ""),
      PayloadField(CType.uint11LE, "channel12", ""),
      PayloadField(CType.uint11LE, "channel13", ""),
      PayloadField(CType.uint11LE, "channel14", ""),
      PayloadField(CType.uint11LE, "channel15", ""),
    ],
  ),
  battery_sensor(0x08, [
    PayloadField(CType.uint16, "voltage", "voltage in dV (Big Endian)"),
    PayloadField(CType.uint16, "current", "current in dA (Big Endian)"),
    PayloadField(CType.uint32, "capacity", "used capacity in mAh",
        writeType: CType.uint24),
    PayloadField(
        CType.uint8, "percent", "estimated battery remaining in percent (%)"),
  ]),
  link_statistics(0x14, [
    PayloadField(
        CType.uint8, "uplink_rssi_ant_1", "Uplink RSSI Ant. 1 ( dBm * -1 )"),
    PayloadField(
        CType.uint8, "uplink_rssi_ant_2", "Uplink RSSI Ant. 2 ( dBm * -1 )"),
    // Uplink LQ of 0 may used to indicate a disconnected status to the handset
    PayloadField(CType.uint8, "uplink_package_success_rate",
        "Uplink Package success rate / Link quality ( % )"),
    PayloadField(CType.int8, "uplink_snr",
        "Uplink SNR ( dB, or dB*4 for TBS I believe )"),
    PayloadField(CType.uint8, "diversity_active_antenna",
        "Diversity active antenna ( enum ant. 1 = 0, ant. 2 = 1 )"),
    PayloadField(CType.uint8, "rf_mode",
        "RF Mode ( 500Hz, 250Hz etc, varies based on ELRS Band or TBS )"),
    PayloadField(CType.uint8, "uplink_tx_power",
        "Uplink TX Power ( enum 0mW = 0, 10mW, 25 mW, 100 mW, 500 mW, 1000 mW, 2000mW, 50mW )"),
    PayloadField(CType.uint8, "downlink_rssi", "Downlink RSSI ( dBm * -1 )"),
    PayloadField(CType.uint8, "downlink_package_success_rate",
        "Downlink package success rate / Link quality ( % )"),
    PayloadField(CType.int8, "downlink_snr", "Downlink SNR ( dB )"),
  ]);

  const Payload(this.frameType, this.fields);

  final int frameType;
  final List<PayloadField> fields;

  int get length {
    int len = 0;
    for (var field in fields) {
      len += field.type == CType.uint24 ? 3 : 2;
    }
    return len;
  }

  void toStruct(StringBuffer buffer) {
    final packed = fields.any((field) => field.type.packed);
    buffer.write("typedef struct ");
    if (packed) {
      buffer.write("__attribute__((packed)) ");
    }
    buffer.write("{");
    buffer.writeln();
    for (var field in fields) {
      final hasComment = field.description.isNotEmpty;
      if (hasComment) {
        buffer.write("\t// ${field.description}");
        buffer.writeln();
      }
      buffer.write("\t");
      buffer.write(field.type);
      buffer.write(" ");
      buffer.write(field.name);
      if (field.type.packed) {
        buffer.write(" : ${field.type.bits}");
      }
      buffer.write(";");
      buffer.writeln();
    }
    buffer.write("} crsf_payload_${this.name}_t;");
  }

  void toCBuffer(StringBuffer str) {
    str.write("void _write_${this.name}_payload(crsf_instance *ins)");
    str.writeln();
    str.write("{");
    str.writeln();
    final payloadBits = fields.fold(
      0,
      (int prev, field) => prev + field.writeType.bits,
    );
    final payloadBytes = payloadBits ~/ 8;
    // Type + Payload + CRC
    final frameLength = 1 + payloadBytes + 1;
    _write_to_buffer(str, CType.uint8, "$frameLength");
    str.write("\t// Frame length\n");
    _write_to_buffer(str, CType.uint8, frameTypeEnumName);
    str.write("\t// Frame type\n");
    for (var field in fields) {
      _write_to_buffer(
        str,
        field.writeType,
        "ins->telemetry.${name}.${field.name}",
      );
      str.writeln();
    }
    str.write("}");
  }

  void _write_to_buffer(StringBuffer str, CType type, String arg) {
    String writeFunc = type.writeStr;
    str.write("\t${writeFunc}(&ins->telem_buf, $arg);");
  }

  static void toTelemetryStruct(StringBuffer buffer) {
    buffer.write("typedef struct telemetry_s");
    buffer.writeln();
    buffer.write("{");
    buffer.writeln();
    for (var payload in Payload.values) {
      buffer.write("\tcrsf_payload_${payload.name}_t ${payload.name};");
      buffer.writeln();
    }
    buffer.write("} telemetry_t;");
  }

  static void toEnum(StringBuffer buffer) {
    buffer.write("typedef enum");
    buffer.writeln();
    buffer.write("{");
    buffer.writeln();
    for (var payload in Payload.values) {
      buffer.write(
        "\t${payload.frameTypeEnumName} = ${payload.frameType.hex},",
      );
      buffer.writeln();
    }
    buffer.writeln();
    buffer.write("} frame_type_t;");
  }

  String get frameTypeEnumName {
    return "CRSF_FRAMETYPE_${name.toUpperCase()}";
  }
}
