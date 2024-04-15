package pt.ulisboa.tecnico.socialsoftware.humanaethica.user.repository;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.stereotype.Repository;
import org.springframework.transaction.annotation.Transactional;
import pt.ulisboa.tecnico.socialsoftware.humanaethica.assessment.domain.Assessment;
import pt.ulisboa.tecnico.socialsoftware.humanaethica.participation.domain.Participation;
import pt.ulisboa.tecnico.socialsoftware.humanaethica.user.domain.User;

import java.util.List;

@Repository
@Transactional
public interface UserRepository extends JpaRepository<User, Integer> {
    @Query("SELECT p FROM Participation p WHERE p.volunteer.id = :UserId")
    List<Participation> getVolunteerParticipations(Integer UserId);
    @Query("SELECT a FROM Assessment a WHERE a.volunteer.id = :UserId")
    List<Assessment> getVolunteerAssessments(Integer UserId);
}