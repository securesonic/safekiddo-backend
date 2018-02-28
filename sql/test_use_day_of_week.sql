-- parent
insert into parent (id, name) values
(1, 'Ojciec Zosi');

-- profile
insert into profile (id, name, owner_code_parent_id) values
(20, 'Profil Zosi', 1);

-- child
insert into child (id, name, birth_date, code_parent_id, uuid, default_custom_code_profile_id) values
(37, 'Zosia', '2006-01-01', 1, '70514', 20);

-- device
insert into device (id, code_child_id, name, serial) values
(1, 37, 'Tablet Zosi', 123456);

-- access control
insert into access_control (id, code_child_id, day_of_week, start_time, end_time) values
(1, 37, 1, '10:00', '18:00'),
(2, 37, 2, '10:00', '18:00'),
(3, 37, 3, '10:00', '18:00'),
(4, 37, 4, '10:00', '18:00'),
(5, 37, 5, '10:00', '18:00'),
(6, 37, 6, '08:00', '20:00'),
(0, 37, 0, '08:00', '20:00');

-- categories_groups
insert into categories_groups (id, name) values
(27, 'Nieskategoryzowane');
