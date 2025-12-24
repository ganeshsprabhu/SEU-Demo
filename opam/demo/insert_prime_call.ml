open Cil

(* -------------------------------------------------- *)
(* Helpers                                            *)
(* -------------------------------------------------- *)

let prime_name name = name ^ "_prime"

let prime_lval (lv : lval) : lval =
  match lv with
  | (Var vi, off) ->
      let vi' = makeGlobalVar (prime_name vi.vname) vi.vtype in
      (Var vi', off)
  | _ ->
      lv  (* untouched; we assume simple cases *)

let prime_exp (e : exp) : exp =
  match e with
  | Lval (Var vi, off) ->
      Lval (Var (makeGlobalVar (prime_name vi.vname) vi.vtype), off)
  | _ ->
      e

let prime_fun_exp (e : exp) : exp =
  match e with
  | Lval (Var vi, NoOffset) ->
      let vi' = makeGlobalVar (prime_name vi.vname) vi.vtype in
      Lval (Var vi', NoOffset)
  | _ -> e

(* -------------------------------------------------- *)
(* Visitor                                           *)
(* -------------------------------------------------- *)

class insertPrimeCallVisitor (target_fun : string) = object
  inherit nopCilVisitor

  val mutable inserted = false

  method! vstmt s =
    match s.skind with
    | Instr instrs ->
        let new_instrs = ref [] in

        List.iter (fun instr ->
          new_instrs := !new_instrs @ [instr];

          match instr with
          | Call (lhs, fn, args, loc)
            when not inserted ->
              (match fn with
               | Lval (Var vi, NoOffset)
                 when vi.vname = target_fun ->

                   inserted <- true;

                   let lhs' =
                     match lhs with
                     | Some lv -> Some (prime_lval lv)
                     | None -> None
                   in

                   let fn' = prime_fun_exp fn in
                   let args' = List.map prime_exp args in

                   let new_call =
                     Call (lhs', fn', args', loc)
                   in

                   new_instrs := !new_instrs @ [new_call]

               | _ -> ())
          | _ -> ()
        ) instrs;

        ChangeTo (mkStmt (Instr !new_instrs))

    | _ -> DoChildren
end

(* -------------------------------------------------- *)
(* Main                                              *)
(* -------------------------------------------------- *)

let () =
  if Array.length Sys.argv <> 3 then begin
    Printf.eprintf "Usage: %s <input.c> <function_name>\n" Sys.argv.(0);
    exit 1
  end;

  let input_file = Sys.argv.(1) in
  let target_fun = Sys.argv.(2) in

  let file = Frontc.parse input_file () in
  let visitor = new insertPrimeCallVisitor target_fun in

  visitCilFileSameGlobals visitor file;

  dumpFile defaultCilPrinter stdout input_file file

