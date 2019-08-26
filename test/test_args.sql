CREATE OR REPLACE FUNCTION rint4(i int4) RETURNS int4 AS $$
return (as.integer(i))
$$ LANGUAGE plr;
select rint4(12345678);

create or replace function out_args(out i int, out j float) as $$  
$$ language gp_compute;
select out_args();

create or replace function composite_arg(p pg_type) returns void as $$ 
$$ language gp_compute;
select composite_args(pg_type) from pg_type limit 1;
