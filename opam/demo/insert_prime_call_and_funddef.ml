open Cil

(* -------------------------------------------------- *)
(* Compatibility helper (OCaml < 4.13)                *)
(* -------------------------------------------------- *)

let rec find_map f = function
  | [] -> None
  | x :: xs ->
      match f x with
      | Some _ as r -> r
      | None -> find_map f xs

(* -------------------------------------------------- *)
(* Helpers                                            *)
(* -------------------------------------------------- *)

let prime_name s = s ^ "_prime"

let prime_lval (lv : lval) : lval =
  match lv with
  | (Var vi, off) ->
      let vi' = makeGlobalVar (prime_name vi.vname) vi.vtype in
      (Var vi', off)
  | _ -> lv

let prime_exp (e : exp) : exp =
  match e with
  | Lval (Var vi, off) ->
      Lval (Var (makeGlobalVar (prime_name vi.vname) vi.vtype), off)
  | _ -> e

let prime_fun_exp (e : exp) : exp =
  match e with
  | Lval (Var vi, NoOffset) ->
      let vi' = makeGlobalVar (prime_name vi.vname) vi.vtype in
      Lval (Var vi', NoOffset)
  | _ -> e

(* -------------------------------------------------- *)
(* Visitor: insert _prime call                        *)
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

                   let new_call = Call (lhs', fn', args', loc) in
                   new_instrs := !new_instrs @ [new_call]
               | _ -> ())
          | _ -> ()
        ) instrs;

        ChangeTo (mkStmt (Instr !new_instrs))

    | _ -> DoChildren
end

(* -------------------------------------------------- *)
(* Create _prime function definition                  *)
(* -------------------------------------------------- *)

let make_prime_function (orig : fundec) : global =
  let fdec = emptyFunction (prime_name orig.svar.vname) in
  fdec.svar.vtype <- orig.svar.vtype;

  fdec.sformals <-
    List.map (fun vi ->
      makeFormalVar fdec (prime_name vi.vname) vi.vtype
    ) orig.sformals;

  let ret_stmt =
    mkStmt (Return (Some zero, locUnknown))
  in
  fdec.sbody <- mkBlock [ret_stmt];

  GFun (fdec, locUnknown)

(* -------------------------------------------------- *)
(* Insert function after all #includes                *)
(* -------------------------------------------------- *)

let insert_after_includes file new_fun =
  let rec split acc = function
    | (GText _ as g) :: rest -> split (acc @ [g]) rest
    | rest -> acc, rest
  in
  let includes, rest = split [] file.globals in
  file.globals <- includes @ [new_fun] @ rest

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

  let orig_fun =
    match find_map (function
      | GFun (fd, _) when fd.svar.vname = target_fun -> Some fd
      | _ -> None
    ) file.globals with
    | Some fd -> fd
    | None -> failwith ("Function not found: " ^ target_fun)
  in

  let prime_fun = make_prime_function orig_fun in
  insert_after_includes file prime_fun;

  let visitor = new insertPrimeCallVisitor target_fun in
  visitCilFileSameGlobals visitor file;

  dumpFile defaultCilPrinter stdout input_file file

