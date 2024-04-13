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
  uint8,
  uint16,
  uint24,
  uint32,
  int8,
  int16,
  int32;

  String toString() {
    switch (this) {
      case CType.uint8:
        return "uint8_t";
      case CType.uint16:
        return "uint16_t";
      case CType.uint24:
        return "uint24_t";
      case CType.uint32:
        return "uint32_t";
      case CType.int8:
        return "int8_t";
      case CType.int16:
        return "int16_t";
      case CType.int32:
        return "int32_t";
    }
  }

  String toWriteString() {
    switch (this) {
      case CType.uint8:
        return "sb_write_ui8";
      case CType.uint16:
        return "sb_write_ui16";
      case CType.uint24:
        return "sb_write_ui24";
      case CType.uint32:
        return "sb_write_ui32";
      case CType.int8:
        return "sb_write_i8";
      case CType.int16:
        return "sb_write_i16";
      case CType.int32:
        return "sb_write_i32";
    }
  }

  int get sizeInBytes {
    switch (this) {
      case CType.uint8:
      case CType.int8:
        return 1;
      case CType.uint16:
      case CType.int16:
        return 2;
      case CType.uint24:
        return 3;
      case CType.uint32:
      case CType.int32:
        return 4;
    }
  }
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
    buffer.write("typedef struct crsf_${this.name}_s");
    buffer.writeln();
    buffer.write("{");
    buffer.writeln();
    for (var field in fields) {
      buffer.write("\t// ${field.description}");
      buffer.writeln();
      buffer.write("\t");
      buffer.write(field.type);
      buffer.write(" ");
      buffer.write(field.name);
      buffer.write(";");
      buffer.writeln();
    }
    buffer.write("} crsf_payload_${this.name}_t;");
  }

  void toCBuffer(StringBuffer str) {
    str.write("void _write_${this.name}_payload()");
    str.writeln();
    str.write("{");
    str.writeln();
    final payloadBytes = fields.fold(
      0,
      (int prev, field) => prev + field.writeType.sizeInBytes,
    );
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
        "_telemetry.${name}.${field.name}",
      );
      str.writeln();
    }
    str.write("}");
  }

  void _write_to_buffer(StringBuffer str, CType type, String arg) {
    String writeFunc = type.toWriteString();
    str.write("\t${writeFunc}(&_telemBuf, $arg);");
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
    buffer.write("typedef enum frame_type_e");
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
