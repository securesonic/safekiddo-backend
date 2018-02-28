-- parent
insert into parent (id, name) values
($parentId, 'Ojciec Tomka-$childId');

-- profile
insert into profile (id, name, owner_code_parent_id) values
($profileId, 'Profil Tomka', $parentId);

-- child
insert into child (id, name, birth_date, code_parent_id, uuid, default_custom_code_profile_id) values
($childId, 'Tomek-$childId', '$birthDate', $parentId, '$uuid', $profileId);

-- device
insert into device (code_child_id, name, serial) values
($childId, 'Tablet Tomka-$childId', $serial);

-- access control
insert into access_control (code_child_id, day_of_week, start_time, end_time) values
($childId, 0, '8:00', '18:00'), -- Sun
($childId, 1, '8:00', '18:00'),
($childId, 2, '8:00', '18:00'),
($childId, 3, '8:00', '18:00'),
($childId, 4, '8:00', '18:00'),
($childId, 5, '8:00', '18:00'),
($childId, 6, '8:00', '18:00'); -- Sat

-- profile categories groups list
insert into profile_categories_groups_list (code_profile_id, code_categories_groups_id, blocked) values
($profileId, 43, true); -- Adult blocked
