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
(0, 37, 0, '8:00', '18:00'),
(1, 37, 1, '8:00', '18:00'),
(2, 37, 2, '8:00', '18:00'),
(3, 37, 3, '8:00', '18:00'),
(4, 37, 4, '8:00', '18:00'),
(5, 37, 5, '8:00', '18:00'),
(6, 37, 6, '8:00', '18:00');

-- categories_groups
insert into categories_groups (id, name) values
(42, 'Illegal'),
(43, 'Adult'),
(46,'Portale'),
(27, 'Nieskategoryzowane');

-- categories
insert into categories (ext_id, name, code_categories_groups_id) values
(11, 'Adult and pornography', 43),
(10, 'Abused drugs', 42),
(48, 'Violence', 43),
(64, 'Illegal', 42),
(51,'Internet Portals',46);

-- age profile: not used in backend
--insert into age_profile (id, from_age, to_age, code_profile_id) values

-- profile categories groups list
insert into profile_categories_groups_list (id, code_profile_id, code_categories_groups_id, blocked) values
(1,20,43, true);
