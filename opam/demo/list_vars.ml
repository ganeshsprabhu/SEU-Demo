open Cil

module StringSet = Set.Make(String)

let () =
  if Array.length Sys.argv <> 4 then begin
    Printf.eprintf
      "Usage: %s <input.c> <function_name> <output_file>\n"
      Sys.argv.(0);
    exit 1
  end;

  let input_file  = Sys.argv.(1) in
  let target_fun  = Sys.argv.(2) in
  let output_file = Sys.argv.(3) in

  let file = Frontc.parse input_file () in
  let found = ref false in

  iterGlobals file (function
    | GFun (fd, _) when fd.svar.vname = target_fun ->
        found := true;

        (* Collect unique variable names *)
        let vars =
          List.fold_left
            (fun acc vi -> StringSet.add vi.vname acc)
            StringSet.empty
            (fd.sformals @ fd.slocals)
        in

        (* Write to output file *)
        let oc = open_out output_file in
        StringSet.iter (fun vname ->
          output_string oc (vname ^ "\n")
        ) vars;
        close_out oc;

        Printf.printf
          "Written %d variables to %s\n"
          (StringSet.cardinal vars)
          output_file

    | _ -> ()
  );

  if not !found then
    Printf.printf "Function '%s' not found.\n" target_fun

