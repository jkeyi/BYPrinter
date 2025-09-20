program enum_printproc;

{$APPTYPE CONSOLE}

uses
  SysUtils, Windows, WinSpool;

procedure enum_PrintProcessorDatatypes(ProcessorName: PChar);
var
  buffer: Pointer;
  cbNeeded, cReturned, I: DWORD;
  dti: ^DATATYPES_INFO_1;
begin
  if not EnumPrintProcessorDatatypes(nil, ProcessorName, 1, nil, 0, cbNeeded, cReturned) and
  (GetLastError = ERROR_INSUFFICIENT_BUFFER) then begin
    GetMem(buffer, cbNeeded);
    try
      if EnumPrintProcessorDatatypes(nil, ProcessorName, 1, buffer, cbNeeded, cbNeeded, cReturned) then begin
        Cardinal(dti) := Cardinal(buffer);
        for I := 1 to cReturned do begin
          Writeln('  ', dti^.pName);
          Inc(dti);
        end;
      end;
    finally
      FreeMem(buffer);
    end;
  end;
end;

procedure enum_PrintProcessors;
var
  buffer: Pointer;
  cbNeeded, cReturned, I: DWORD;
  ppi: ^PRINTPROCESSOR_INFO_1;
begin
  if not EnumPrintProcessors(nil, nil, 1, nil, 0, cbNeeded, cReturned) and
  (GetLastError = ERROR_INSUFFICIENT_BUFFER) then begin
    GetMem(buffer, cbNeeded);
    try
      if EnumPrintProcessors(nil, nil, 1, buffer, cbNeeded, cbNeeded, cReturned) then begin
        Cardinal(ppi) := Cardinal(buffer);
        for I := 1 to cReturned do begin
          Writeln(ppi^.pName);
          enum_PrintProcessorDatatypes(ppi^.pName);
          Inc(ppi);
        end;
      end;
    finally
      FreeMem(buffer);
    end;
  end;
end;

begin
  try
    enum_PrintProcessors;
  except
    on E: Exception do
      Writeln(E.ClassName, ': ', E.Message);
  end;
end.
