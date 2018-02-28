insert into parent (name) values ('ICAP parent');

insert into profile (name, owner_code_parent_id) values 
('ICAP profile', currval(pg_get_serial_sequence('public.parent', 'id')));

insert into profile_categories_groups_list (code_profile_id, code_categories_groups_id, blocked) values (currval(pg_get_serial_sequence('public.profile', 'id')), 3, 'true');

insert into child (name, birth_date, code_parent_id, uuid, default_custom_code_profile_id) 
	select 'ICAPChild', '2014-01-01', currval(pg_get_serial_sequence('public.parent', 'id')), '70514-' || id.num, currval(pg_get_serial_sequence('public.profile', 'id')) FROM generate_series(0,19) AS id(num);

insert into access_control (code_child_id, day_of_week, start_time, end_time) 
	select currval(pg_get_serial_sequence('public.child', 'id')) - id.num, day.num, '00:00:00', '00:00:00' FROM generate_series(0,19) AS id(num), generate_series(0,6) AS day(num); 
