describe('Assessment', () => {
  beforeEach(() => {
    cy.deleteAllButArs();
    cy.createAssessments();
  });

  afterEach(() => {
    cy.deleteAllButArs();
  });

  it('assess an institution and check if it exists', () => {
    const NAME = 'A1'; //name of the first assessment by alphabetical order
    const REVIEW = 'My review is this.';

    cy.demoVolunteerLogin();

    //to allow waiting for the page to load
    cy.intercept('GET', '/activities').as('getActivities')
    cy.intercept('POST','/institutions/*/assessments').as('saveAssessment')

    //go to activities page
    cy.get('[data-cy="volunteerActivities"]').click();
    cy.wait('@getActivities')

    //check if the table has 6 rows from the 6 added activities
    cy.get('[data-cy="volunteerActivitiesTable"] tbody tr')
        .should('have.length', 6)
        .eq(0)

    //check if the first row/item has the correct name
    cy.get('[data-cy="volunteerActivitiesTable"] tbody tr')
        .eq(0).children().eq(0).should('contain', NAME)

    //Assess the first activity
    cy.get('[data-cy="volunteerActivitiesTable"] tbody tr')
        .eq(0).children().eq(9).get('[data-cy="assessButton"]').click();
    cy.get('[data-cy="reviewInput"]').type(REVIEW);
    cy.get('[data-cy="saveButton"]').click();
    cy.wait('@saveAssessment')

    cy.logout();

    cy.demoMemberLogin();

    cy.intercept('GET', '/institutions/*/assessments').as('getAssessments')

    //go to assessments page
    cy.get('[data-cy="institution"]').click();
    cy.get('[data-cy="assessments"]').click();
    cy.wait('@getAssessments')

    cy.get('[data-cy="institutionAssessmentsTable"] tbody tr')
      .should('have.length', 1)
      .eq(0)

    cy.get('[data-cy="institutionAssessmentsTable"] tbody tr')
      .eq(0).children().eq(0).should('contain', REVIEW)

    cy.logout();
  });
});
