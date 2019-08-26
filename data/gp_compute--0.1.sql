CREATE OR REPLACE FUNCTION gpc_client_handle_call() RETURNS language_handler AS 
'$libdir/gp_compute_client'
LANGUAGE C;

CREATE TRUSTED LANGUAGE gp_compute HANDLER gpc_client_handle_call;