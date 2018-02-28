insert into parent (name) values ('Monit parent');

insert into profile (name, owner_code_parent_id) values 
('Monit profile', currval(pg_get_serial_sequence('public.parent', 'id')));

insert into child (name, birth_date, code_parent_id, uuid, default_custom_code_profile_id) values 
('Monit', '2014-01-01', currval(pg_get_serial_sequence('public.parent', 'id')), '70514', currval(pg_get_serial_sequence('public.profile', 'id')));

insert into profile_categories_groups_list (code_profile_id, code_categories_groups_id, blocked) values (currval(pg_get_serial_sequence('public.profile', 'id')), 3, 'true');

insert into access_control (code_child_id, day_of_week, start_time, end_time) values 
(currval(pg_get_serial_sequence('public.child', 'id')), 0, '00:00:00', '00:00:00'),
(currval(pg_get_serial_sequence('public.child', 'id')), 1, '00:00:00', '00:00:00'),
(currval(pg_get_serial_sequence('public.child', 'id')), 2, '00:00:00', '00:00:00'),
(currval(pg_get_serial_sequence('public.child', 'id')), 3, '00:00:00', '00:00:00'),
(currval(pg_get_serial_sequence('public.child', 'id')), 4, '00:00:00', '00:00:00'),
(currval(pg_get_serial_sequence('public.child', 'id')), 5, '00:00:00', '00:00:00'),
(currval(pg_get_serial_sequence('public.child', 'id')), 6, '00:00:00', '00:00:00');
